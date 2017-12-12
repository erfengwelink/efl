#include"sgmatrix.h"
#include<cmath>
#include<cstring>
#include<cassert>
#include<sgglobal.h>

struct SGMatrixData {
    RefCount             ref;
    SGMatrix::MatrixType type;
    SGMatrix::MatrixType dirty;
    float m11, m12, m13;
    float m21, m22, m23;
    float mtx, mty, m33;
};
static const struct SGMatrixData shared_empty = {RefCount(-1),
                                                 SGMatrix::MatrixType::None,
                                                 SGMatrix::MatrixType::None,
                                                 1, 0, 0,
                                                 0, 1, 0,
                                                 0, 0, 1};
inline float SGMatrix::determinant() const
{    
    return d->m11*(d->m33*d->m22 - d->mty*d->m23) -
           d->m21*(d->m33*d->m12 - d->mty*d->m13)+d->mtx*(d->m23*d->m12 - d->m22*d->m13);
}

bool SGMatrix::isAffine() const
{
    return type() < MatrixType::Project;
}

bool SGMatrix::isIdentity() const
{
    return type() == MatrixType::None;
}

bool SGMatrix::isInvertible() const
{
    return !sgIsNull(determinant());
}

bool SGMatrix::isScaling() const
{
    return type() >= MatrixType::Scale;
}
bool SGMatrix::isRotating() const
{
    return type() >= MatrixType::Rotate;
}

bool SGMatrix::isTranslating() const
{
    return type() >= MatrixType::Translate;
}

inline void SGMatrix::cleanUp(SGMatrixData *d)
{
    delete d;
}

void SGMatrix::detach()
{
    if (d->ref.isShared())
        *this = copy();
}

SGMatrix SGMatrix::copy() const
{
    SGMatrix r;

    r.d = new SGMatrixData;
    memcpy(r.d, d, sizeof(SGMatrixData));
    r.d->ref.setOwned();
    return r;
}

SGMatrix::SGMatrix()
    : d(const_cast<SGMatrixData*>(&shared_empty))
{
}

SGMatrix::~SGMatrix()
{
    if (!d->ref.deref())
        cleanUp(d);
}

SGMatrix::SGMatrix(bool init SG_UNUSED)
{
    d = new SGMatrixData;
    memcpy(d, &shared_empty, sizeof(SGMatrixData));
    d->ref.setOwned();
}

SGMatrix::SGMatrix(float h11, float h12, float h13,
                   float h21, float h22, float h23,
                   float h31, float h32, float h33)
{
    d = new SGMatrixData;
    d->ref.setOwned();
    d->m11 = h11; d->m12 = h12; d->m13 = h13;
    d->m21 = h21; d->m22 = h22; d->m23 = h23;
    d->mtx = h31; d->mty = h32; d->m33 = h33;
    d->type = MatrixType::None;
    d->dirty = MatrixType::Project;
}

SGMatrix::SGMatrix(const SGMatrix &m)
{
    d = m.d;
    d->ref.ref();
}

SGMatrix::SGMatrix(SGMatrix &&other): d(other.d)
{
    other.d = const_cast<SGMatrixData*>(&shared_empty);
}

SGMatrix &SGMatrix::operator=(const SGMatrix &m)
{
    m.d->ref.ref();
    if (!d->ref.deref())
        cleanUp(d);

    d = m.d;
    return *this;
}

inline SGMatrix &SGMatrix::operator=(SGMatrix &&other)
{
    std::swap(d, other.d); return *this;
}

inline SGMatrix &SGMatrix::operator*=(float num)
{
    if (num == 1.)
        return *this;
    detach();
    d->m11 *= num;
    d->m12 *= num;
    d->m13 *= num;
    d->m21 *= num;
    d->m22 *= num;
    d->m23 *= num;
    d->mtx *= num;
    d->mty *= num;
    d->m33 *= num;
    if (d->dirty < MatrixType::Scale)
        d->dirty = MatrixType::Scale;

    return *this;
}

inline SGMatrix &SGMatrix::operator/=(float div)
{
    if (div == 0)
        return *this;
    detach();
    div = 1/div;
    return operator*=(div);
}

SGMatrix::MatrixType SGMatrix::type() const
{
    if(d->dirty == MatrixType::None || d->dirty < d->type)
        return static_cast<MatrixType>(d->type);

    switch (static_cast<MatrixType>(d->dirty)) {
    case MatrixType::Project:
        if (!sgIsNull(d->m13) || !sgIsNull(d->m23) || !sgIsNull(d->m33 - 1)) {
             d->type = MatrixType::Project;
             break;
        }
    case MatrixType::Shear:
    case MatrixType::Rotate:
        if (!sgIsNull(d->m12) || !sgIsNull(d->m21)) {
            const float dot = d->m11 * d->m12 + d->m21 * d->m22;
            if (sgIsNull(dot))
                d->type = MatrixType::Rotate;
            else
                d->type = MatrixType::Shear;
            break;
        }
    case MatrixType::Scale:
        if (!sgIsNull(d->m11 - 1) || !sgIsNull(d->m22 - 1)) {
            d->type = MatrixType::Scale;
            break;
        }
    case MatrixType::Translate:
        if (!sgIsNull(d->mtx) || !sgIsNull(d->mty)) {
            d->type = MatrixType::Translate;
            break;
        }
    case MatrixType::None:
        d->type = MatrixType::None;
        break;
    }

    d->dirty = MatrixType::None;
    return static_cast<MatrixType>(d->type);
}


SGMatrix &SGMatrix::translate(float dx, float dy)
{
    if (dx == 0 && dy == 0)
        return *this;
    detach();
    switch(type()) {
    case MatrixType::None:
        d->mtx = dx;
        d->mty = dy;
        break;
    case MatrixType::Translate:
        d->mtx += dx;
        d->mty += dy;
        break;
    case MatrixType::Scale:
        d->mtx += dx* d->m11;
        d->mty += dy* d->m22;
        break;
    case MatrixType::Project:
        d->m33 += dx * d->m13 + dy * d->m23;
    case MatrixType::Shear:
    case MatrixType::Rotate:
        d->mtx += dx*d->m11 + dy*d->m21;
        d->mty += dy*d->m22 + dx*d->m12;
        break;
    }
    if (d->dirty < MatrixType::Translate)
        d->dirty = MatrixType::Translate;
    return *this;
}

SGMatrix & SGMatrix::scale(float sx, float sy)
{
    if (sx == 1 && sy == 1)
        return *this;
    detach();
    switch(type()) {
    case MatrixType::None:
    case MatrixType::Translate:
        d->m11 = sx;
        d->m22 = sy;
        break;
    case MatrixType::Project:
        d->m13 *= sx;
        d->m23 *= sy;
    case MatrixType::Rotate:
    case MatrixType::Shear:
        d->m12 *= sx;
        d->m21 *= sy;
    case MatrixType::Scale:
        d->m11 *= sx;
        d->m22 *= sy;
        break;
    }
    if (d->dirty < MatrixType::Scale)
        d->dirty =  MatrixType::Scale;
    return *this;
}

SGMatrix & SGMatrix::shear(float sh, float sv)
{
    if (sh == 0 && sv == 0)
        return *this;
    detach();
    switch(type()) {
    case MatrixType::None:
    case MatrixType::Translate:
        d->m12 = sv;
        d->m21 = sh;
        break;
    case MatrixType::Scale:
        d->m12 = sv*d->m22;
        d->m21 = sh*d->m11;
        break;
    case MatrixType::Project: {
        float tm13 = sv*d->m23;
        float tm23 = sh*d->m13;
        d->m13 += tm13;
        d->m23 += tm23;
    }
    case MatrixType::Rotate:
    case MatrixType::Shear: {
        float tm11 = sv*d->m21;
        float tm22 = sh*d->m12;
        float tm12 = sv*d->m22;
        float tm21 = sh*d->m11;
        d->m11 += tm11; d->m12 += tm12;
        d->m21 += tm21; d->m22 += tm22;
        break;
    }
    }
    if (d->dirty < MatrixType::Shear)
        d->dirty = MatrixType::Shear;
    return *this;
}


static const float deg2rad = float(0.017453292519943295769);  // pi/180
static const float inv_dist_to_plane = 1. / 1024.;

SGMatrix & SGMatrix::rotate(float a, Axis axis)
{
    if (a == 0)
        return *this;
    detach();
    float sina = 0;
    float cosa = 0;
    if (a == 90. || a == -270.)
        sina = 1.;
    else if (a == 270. || a == -90.)
        sina = -1.;
    else if (a == 180.)
        cosa = -1.;
    else{
        float b = deg2rad*a;          // convert to radians
        sina = std::sin(b);               // fast and convenient
        cosa = std::cos(b);
    }

    if (axis == Axis::Z) {
        switch(type()) {
        case MatrixType::None:
        case MatrixType::Translate:
            d->m11 = cosa;
            d->m12 = sina;
            d->m21 = -sina;
            d->m22 = cosa;
            break;
        case MatrixType::Scale: {
            float tm11 = cosa*d->m11;
            float tm12 = sina*d->m22;
            float tm21 = -sina*d->m11;
            float tm22 = cosa*d->m22;
            d->m11 = tm11; d->m12 = tm12;
            d->m21 = tm21; d->m22 = tm22;
            break;
        }
        case MatrixType::Project: {
            float tm13 = cosa*d->m13 + sina*d->m23;
            float tm23 = -sina*d->m13 + cosa*d->m23;
            d->m13 = tm13;
            d->m23 = tm23;
        }
        case MatrixType::Rotate:
        case MatrixType::Shear: {
            float tm11 = cosa*d->m11 + sina*d->m21;
            float tm12 = cosa*d->m12 + sina*d->m22;
            float tm21 = -sina*d->m11 + cosa*d->m21;
            float tm22 = -sina*d->m12 + cosa*d->m22;
            d->m11 = tm11; d->m12 = tm12;
            d->m21 = tm21; d->m22 = tm22;
            break;
        }
        }
        if (d->dirty < MatrixType::Rotate)
            d->dirty = MatrixType::Rotate;
    } else {
        SGMatrix result;
        if (axis == Axis::Y) {
            result.d->m11 = cosa;
            result.d->m13 = -sina * inv_dist_to_plane;
        } else {
            result.d->m22 = cosa;
            result.d->m23 = -sina * inv_dist_to_plane;
        }
        result.d->type = MatrixType::Project;
        *this = result * *this;
    }

    return *this;
}

SGMatrix SGMatrix::operator*(const SGMatrix &m) const
{
    const MatrixType otherType = m.type();
    if (otherType == MatrixType::None)
        return *this;

    const MatrixType thisType = type();
    if (thisType == MatrixType::None)
        return m;

    SGMatrix t(true);
    MatrixType type = sgMax(thisType, otherType);
    switch(type) {
    case MatrixType::None:
        break;
    case MatrixType::Translate:
        t.d->mtx = d->mtx + m.d->mtx;
        t.d->mty += d->mty + m.d->mty;
        break;
    case MatrixType::Scale:
    {
        float m11 = d->m11*m.d->m11;
        float m22 = d->m22*m.d->m22;

        float m31 = d->mtx*m.d->m11 + m.d->mtx;
        float m32 = d->mty*m.d->m22 + m.d->mty;

        t.d->m11 = m11;
        t.d->m22 = m22;
        t.d->mtx = m31; t.d->mty = m32;
        break;
    }
    case MatrixType::Rotate:
    case MatrixType::Shear:
    {
        float m11 = d->m11*m.d->m11 + d->m12*m.d->m21;
        float m12 = d->m11*m.d->m12 + d->m12*m.d->m22;

        float m21 = d->m21*m.d->m11 + d->m22*m.d->m21;
        float m22 = d->m21*m.d->m12 + d->m22*m.d->m22;

        float m31 = d->mtx*m.d->m11 + d->mty*m.d->m21 + m.d->mtx;
        float m32 = d->mtx*m.d->m12 + d->mty*m.d->m22 + m.d->mty;

        t.d->m11 = m11; t.d->m12 = m12;
        t.d->m21 = m21; t.d->m22 = m22;
        t.d->mtx = m31; t.d->mty = m32;
        break;
    }
    case MatrixType::Project:
    {
        float m11 = d->m11*m.d->m11 + d->m12*m.d->m21 + d->m13*m.d->mtx;
        float m12 = d->m11*m.d->m12 + d->m12*m.d->m22 + d->m13*m.d->mty;
        float m13 = d->m11*m.d->m13 + d->m12*m.d->m23 + d->m13*m.d->m33;

        float m21 = d->m21*m.d->m11 + d->m22*m.d->m21 + d->m23*m.d->mtx;
        float m22 = d->m21*m.d->m12 + d->m22*m.d->m22 + d->m23*m.d->mty;
        float m23 = d->m21*m.d->m13 + d->m22*m.d->m23 + d->m23*m.d->m33;

        float m31 = d->mtx*m.d->m11 + d->mty*m.d->m21 + d->m33*m.d->mtx;
        float m32 = d->mtx*m.d->m12 + d->mty*m.d->m22 + d->m33*m.d->mty;
        float m33 = d->mtx*m.d->m13 + d->mty*m.d->m23 + d->m33*m.d->m33;

        t.d->m11 = m11; t.d->m12 = m12; t.d->m13 = m13;
        t.d->m21 = m21; t.d->m22 = m22; t.d->m23 = m23;
        t.d->mtx = m31; t.d->mty = m32; t.d->m33 = m33;
    }
    }

    t.d->dirty = type;
    t.d->type = type;

    return t;
}

SGMatrix & SGMatrix::operator*=(const SGMatrix &o)
{
    const MatrixType otherType = o.type();
    if (otherType == MatrixType::None)
        return *this;

    const MatrixType thisType = type();
    if (thisType == MatrixType::None)
        return operator=(o);
    detach();
    MatrixType t = sgMax(thisType, otherType);
    switch(t) {
    case MatrixType::None:
        break;
    case MatrixType::Translate:
        d->mtx += o.d->mtx;
        d->mty += o.d->mty;
        break;
    case MatrixType::Scale:
    {
        float m11 = d->m11*o.d->m11;
        float m22 = d->m22*o.d->m22;

        float m31 = d->mtx*o.d->m11 + o.d->mtx;
        float m32 = d->mty*o.d->m22 + o.d->mty;

        d->m11 = m11;
        d->m22 = m22;
        d->mtx = m31; d->mty = m32;
        break;
    }
    case MatrixType::Rotate:
    case MatrixType::Shear:
    {
        float m11 = d->m11*o.d->m11 + d->m12*o.d->m21;
        float m12 = d->m11*o.d->m12 + d->m12*o.d->m22;

        float m21 = d->m21*o.d->m11 + d->m22*o.d->m21;
        float m22 = d->m21*o.d->m12 + d->m22*o.d->m22;

        float m31 = d->mtx*o.d->m11 + d->mty*o.d->m21 + o.d->mtx;
        float m32 = d->mtx*o.d->m12 + d->mty*o.d->m22 + o.d->mty;

        d->m11 = m11; d->m12 = m12;
        d->m21 = m21; d->m22 = m22;
        d->mtx = m31; d->mty = m32;
        break;
    }
    case MatrixType::Project:
    {
        float m11 = d->m11*o.d->m11 + d->m12*o.d->m21 + d->m13*o.d->mtx;
        float m12 = d->m11*o.d->m12 + d->m12*o.d->m22 + d->m13*o.d->mty;
        float m13 = d->m11*o.d->m13 + d->m12*o.d->m23 + d->m13*o.d->m33;

        float m21 = d->m21*o.d->m11 + d->m22*o.d->m21 + d->m23*o.d->mtx;
        float m22 = d->m21*o.d->m12 + d->m22*o.d->m22 + d->m23*o.d->mty;
        float m23 = d->m21*o.d->m13 + d->m22*o.d->m23 + d->m23*o.d->m33;

        float m31 = d->mtx*o.d->m11 + d->mty*o.d->m21 + d->m33*o.d->mtx;
        float m32 = d->mtx*o.d->m12 + d->mty*o.d->m22 + d->m33*o.d->mty;
        float m33 = d->mtx*o.d->m13 + d->mty*o.d->m23 + d->m33*o.d->m33;

        d->m11 = m11; d->m12 = m12; d->m13 = m13;
        d->m21 = m21; d->m22 = m22; d->m23 = m23;
        d->mtx = m31; d->mty = m32; d->m33 = m33;
    }
    }

    d->dirty = t;
    d->type = t;

    return *this;
}

SGMatrix SGMatrix::adjoint() const
{
    float h11, h12, h13,
        h21, h22, h23,
        h31, h32, h33;
    h11 = d->m22*d->m33 - d->m23*d->mty;
    h21 = d->m23*d->mtx - d->m21*d->m33;
    h31 = d->m21*d->mty - d->m22*d->mtx;
    h12 = d->m13*d->mty - d->m12*d->m33;
    h22 = d->m11*d->m33 - d->m13*d->mtx;
    h32 = d->m12*d->mtx - d->m11*d->mty;
    h13 = d->m12*d->m23 - d->m13*d->m22;
    h23 = d->m13*d->m21 - d->m11*d->m23;
    h33 = d->m11*d->m22 - d->m12*d->m21;

    return SGMatrix(h11, h12, h13,
                      h21, h22, h23,
                      h31, h32, h33);
}

SGMatrix SGMatrix::inverted(bool *invertible) const
{
    SGMatrix invert(true);
    bool inv = true;

    switch(type()) {
    case MatrixType::None:
        break;
    case MatrixType::Translate:
        invert.d->mtx = -d->mtx;
        invert.d->mty = -d->mty;
        break;
    case MatrixType::Scale:
        inv = !sgIsNull(d->m11);
        inv &= !sgIsNull(d->m22);
        if (inv) {
            invert.d->m11 = 1. / d->m11;
            invert.d->m22 = 1. / d->m22;
            invert.d->mtx = -d->mtx * invert.d->m11;
            invert.d->mty = -d->mty * invert.d->m22;
        }
        break;
    default:
        // general case
        float det = determinant();
        inv = !sgIsNull(det);
        if (inv)
            invert = (adjoint() /= det);
        //TODO Test above line
        break;
    }

    if (invertible)
        *invertible = inv;

    if (inv) {
        // inverting doesn't change the type
        invert.d->type = d->type;
        invert.d->dirty = d->dirty;
    }

    return invert;
}

bool SGMatrix::operator==(const SGMatrix &o) const
{
    return d->m11 == o.d->m11 &&
           d->m12 == o.d->m12 &&
           d->m13 == o.d->m13 &&
           d->m21 == o.d->m21 &&
           d->m22 == o.d->m22 &&
           d->m23 == o.d->m23 &&
           d->mtx == o.d->mtx &&
           d->mty == o.d->mty &&
           d->m33 == o.d->m33;
}

bool SGMatrix::operator!=(const SGMatrix &o) const
{
    return !operator==(o);
}

#define SG_NEAR_CLIP 0.000001
#ifdef MAP
#  undef MAP
#endif
#define MAP(x, y, nx, ny) \
    do { \
        float FX_ = x; \
        float FY_ = y; \
        switch(t) {   \
        case MatrixType::None:  \
            nx = FX_;   \
            ny = FY_;   \
            break;    \
        case MatrixType::Translate:    \
            nx = FX_ + d->mtx;                \
            ny = FY_ + d->mty;                \
            break;                              \
        case MatrixType::Scale:                           \
            nx = d->m11 * FX_ + d->mtx;  \
            ny = d->m22 * FY_ + d->mty;  \
            break;                              \
        case MatrixType::Rotate:                          \
        case MatrixType::Shear:                           \
        case MatrixType::Project:                                      \
            nx = d->m11 * FX_ + d->m21 * FY_ + d->mtx;        \
            ny = d->m12 * FX_ + d->m22 * FY_ + d->mty;        \
            if (t == MatrixType::Project) {                                       \
                float w = ( d->m13 * FX_ + d->m23 * FY_ + d->m33);              \
                if (w < SG_NEAR_CLIP) w = SG_NEAR_CLIP;     \
                w = 1./w;                                               \
                nx *= w;                                                \
                ny *= w;                                                \
            }                                                           \
        }                                                               \
    } while (0)

SGRect SGMatrix::map(const SGRect &rect) const
{
    SGMatrix::MatrixType t = type();
    if (t <= MatrixType::Translate)
        return rect.translated(std::round(d->mtx), std::round(d->mty));

    if (t <= MatrixType::Scale) {
        int x = std::round(d->m11*rect.x() + d->mtx);
        int y = std::round(d->m22*rect.y() + d->mty);
        int w = std::round(d->m11*rect.width());
        int h = std::round(d->m22*rect.height());
        if (w < 0) {
            w = -w;
            x -= w;
        }
        if (h < 0) {
            h = -h;
            y -= h;
        }
        return SGRect(x, y, w, h);
    } else if (t < MatrixType::Project) {
        // see mapToPolygon for explanations of the algorithm.
        float x = 0, y = 0;
        MAP(rect.left(), rect.top(), x, y);
        float xmin = x;
        float ymin = y;
        float xmax = x;
        float ymax = y;
        MAP(rect.right() + 1, rect.top(), x, y);
        xmin = sgMin(xmin, x);
        ymin = sgMin(ymin, y);
        xmax = sgMax(xmax, x);
        ymax = sgMax(ymax, y);
        MAP(rect.right() + 1, rect.bottom() + 1, x, y);
        xmin = sgMin(xmin, x);
        ymin = sgMin(ymin, y);
        xmax = sgMax(xmax, x);
        ymax = sgMax(ymax, y);
        MAP(rect.left(), rect.bottom() + 1, x, y);
        xmin = sgMin(xmin, x);
        ymin = sgMin(ymin, y);
        xmax = sgMax(xmax, x);
        ymax = sgMax(ymax, y);
        return SGRect(std::round(xmin), std::round(ymin), std::round(xmax)-std::round(xmin), std::round(ymax)-std::round(ymin));
    } else {
        // Not supported
        assert(0);
    }
}

SGRegion SGMatrix::map(const SGRegion &r) const
{
    SGMatrix::MatrixType t = type();
    if (t == MatrixType::None)
        return r;

    if (t == MatrixType::Translate) {
        SGRegion copy(r);
        copy.translate(std::round(d->mtx), std::round(d->mty));
        return copy;
    }

    if (t == MatrixType::Scale && r.rectCount() == 1)
        return SGRegion(map(r.boundingRect()));
    // handle mapping of region properly
    assert(0);
    return r;
}

SGPointF SGMatrix::map(const SGPointF &p) const
{
    float fx = p.x();
    float fy = p.y();

    float x = 0, y = 0;

    SGMatrix::MatrixType t = type();
    switch(t) {
    case MatrixType::None:
        x = fx;
        y = fy;
        break;
    case MatrixType::Translate:
        x = fx + d->mtx;
        y = fy + d->mty;
        break;
    case MatrixType::Scale:
        x = d->m11 * fx + d->mtx;
        y = d->m22 * fy + d->mty;
        break;
    case MatrixType::Rotate:
    case MatrixType::Shear:
    case MatrixType::Project:
        x = d->m11 * fx + d->m21 * fy + d->mtx;
        y = d->m12 * fx + d->m22 * fy + d->mty;
        if (t == MatrixType::Project) {
            float w = 1./(d->m13 * fx + d->m23 * fy + d->m33);
            x *= w;
            y *= w;
        }
    }
    return SGPointF(x, y);
}
static std::string type_helper(SGMatrix::MatrixType t)
{
    switch(t) {
    case SGMatrix::MatrixType::None:
        return "MatrixType::None";
        break;
    case SGMatrix::MatrixType::Translate:
        return "MatrixType::Translate";
        break;
    case SGMatrix::MatrixType::Scale:
        return "MatrixType::Scale";
        break;
    case SGMatrix::MatrixType::Rotate:
        return "MatrixType::Rotate";
        break;
    case SGMatrix::MatrixType::Shear:
        return "MatrixType::Shear";
        break;
    case SGMatrix::MatrixType::Project:
        return "MatrixType::Project";
        break;
    }
    return "";
}
std::ostream& operator<<(std::ostream& os, const SGMatrix& o)
{
    os<<"[Matrix: [dptr = "<<o.d<<"]"<<"[ref = "<<o.d->ref.count()<<"]"<<"type ="<<type_helper(o.type())<<"]"<<std::endl;
    return os;
}


