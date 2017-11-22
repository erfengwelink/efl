
#include <iostream>
#include <fstream>

#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <libgen.h>

#include <string>
#include <algorithm>
#include <stdexcept>
#include <iosfwd>
#include <type_traits>
#include <cassert>

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <Eolian.h>

#include <Eina.hh>
#include <Eolian_Cxx.hh>

#include "grammar/klass_def.hpp"
#include "grammar/header.hpp"
#include "grammar/impl_header.hpp"
#include "grammar/types_definition.hpp"

namespace eolian_cxx {

/// Program options.
struct options_type
{
   std::vector<std::string> include_dirs;
   std::vector<std::string> in_files;
   mutable Eolian_Unit const* unit;
   std::string out_file;
   bool main_header;

   options_type() : main_header(false) {}
};

static efl::eina::log_domain domain("eolian_cxx");

static bool
opts_check(eolian_cxx::options_type const& opts)
{
   if (opts.in_files.empty())
     {
        EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
          << "Nothing to generate?" << std::endl;
     }
   else
     {
        return true; // valid.
     }
   return false;
}

static bool
generate(const Eolian_Class* klass, eolian_cxx::options_type const& opts,
         std::string const& cpp_types_header)
{
   std::string header_decl_file_name = opts.out_file.empty()
     ? (::eolian_class_file_get(klass) + std::string(".hh")) : opts.out_file;

   std::string header_impl_file_name = header_decl_file_name;
   std::size_t dot_pos = header_impl_file_name.rfind(".hh");
   if (dot_pos != std::string::npos)
     header_impl_file_name.insert(dot_pos, ".impl");
   else
     header_impl_file_name.insert(header_impl_file_name.size(), ".impl");

   efl::eolian::grammar::attributes::klass_def klass_def(klass, opts.unit);
   std::vector<efl::eolian::grammar::attributes::klass_def> klasses{klass_def};
   std::set<efl::eolian::grammar::attributes::klass_def> forward_klasses{};

   std::set<std::string> c_headers;
   std::set<std::string> cpp_headers;
   c_headers.insert(eolian_class_file_get(klass) + std::string(".h"));
        
   std::function<void(efl::eolian::grammar::attributes::type_def const&)>
     variant_function;
   auto klass_name_function
     = [&] (efl::eolian::grammar::attributes::klass_name const& name)
     {
        Eolian_Class const* klass2 = get_klass(name, opts.unit);
        assert(klass2);
        c_headers.insert(eolian_class_file_get(klass2) + std::string(".h"));
        cpp_headers.insert(eolian_class_file_get(klass2) + std::string(".hh"));
        efl::eolian::grammar::attributes::klass_def cls{klass2, opts.unit};
        forward_klasses.insert(cls);
     };
   auto complex_function
     = [&] (efl::eolian::grammar::attributes::complex_type_def const& complex)
     {
       for(auto&& t : complex.subtypes)
         {
           variant_function(t);
         }
     };
   variant_function = [&] (efl::eolian::grammar::attributes::type_def const& type)
     {
       if(efl::eolian::grammar::attributes::klass_name const*
          name = efl::eina::get<efl::eolian::grammar::attributes::klass_name>
          (&type.original_type))
         klass_name_function(*name);
       else if(efl::eolian::grammar::attributes::complex_type_def const*
              complex = efl::eina::get<efl::eolian::grammar::attributes::complex_type_def>
               (&type.original_type))
         complex_function(*complex);
     };

   std::function<void(Eolian_Class const*)> klass_function
     = [&] (Eolian_Class const* klass2)
     {
       for(efl::eina::iterator<Eolian_Class const> inherit_iterator ( ::eolian_class_inherits_get(klass2))
             , inherit_last; inherit_iterator != inherit_last; ++inherit_iterator)
         {
           Eolian_Class const* inherit = &*inherit_iterator;
           c_headers.insert(eolian_class_file_get(inherit) + std::string(".h"));
           cpp_headers.insert(eolian_class_file_get(inherit) + std::string(".hh"));
           efl::eolian::grammar::attributes::klass_def klass3{inherit, opts.unit};
           forward_klasses.insert(klass3);

           klass_function(inherit);
         }

       efl::eolian::grammar::attributes::klass_def klass2_def(klass2, opts.unit);
       for(auto&& f : klass2_def.functions)
         {
           variant_function(f.return_type);
           for(auto&& p : f.parameters)
             {
               variant_function(p.type);
             }
         }
       for(auto&& e : klass2_def.events)
         {
           if(e.type)
             variant_function(*e.type);
         }
     };
   klass_function(klass);

   cpp_headers.erase(eolian_class_file_get(klass) + std::string(".hh"));

   std::string guard_name;
   as_generator(*(efl::eolian::grammar::string << "_") << efl::eolian::grammar::string << "_EO_HH")
     .generate(std::back_insert_iterator<std::string>(guard_name)
               , std::make_tuple(klass_def.namespaces, klass_def.eolian_name)
               , efl::eolian::grammar::context_null{});

   std::tuple<std::string, std::set<std::string>&, std::set<std::string>&
              , std::string const&
              , std::vector<efl::eolian::grammar::attributes::klass_def>&
              , std::set<efl::eolian::grammar::attributes::klass_def> const&
              , std::vector<efl::eolian::grammar::attributes::klass_def>&
              , std::vector<efl::eolian::grammar::attributes::klass_def>&
              > attributes
   {guard_name, c_headers, cpp_headers, cpp_types_header, klasses, forward_klasses, klasses, klasses};

   if(opts.out_file == "-")
     {
        std::ostream_iterator<char> iterator(std::cout);

        efl::eolian::grammar::class_header.generate(iterator, attributes, efl::eolian::grammar::context_null());
        std::endl(std::cout);

        efl::eolian::grammar::impl_header.generate(iterator, klasses, efl::eolian::grammar::context_null());

        std::endl(std::cout);
        std::flush(std::cout);
        std::flush(std::cerr);
     }
   else
     {
        std::ofstream header_decl;
        header_decl.open(header_decl_file_name);
        if (!header_decl.good())
          {
             EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
               << "Can't open output file: " << header_decl_file_name << std::endl;
             return false;
          }

        std::ofstream header_impl;
        header_impl.open(header_impl_file_name);
        if (!header_impl.good())
          {
             EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
               << "Can't open output file: " << header_impl_file_name << std::endl;
             return false;
          }

        efl::eolian::grammar::class_header.generate
          (std::ostream_iterator<char>(header_decl), attributes, efl::eolian::grammar::context_null());

        efl::eolian::grammar::impl_header.generate
          (std::ostream_iterator<char>(header_impl), klasses, efl::eolian::grammar::context_null());

        header_impl.close();
        header_decl.close();
     }
   return true;
}

static bool
types_generate(std::string const& fname, options_type const& opts,
               std::string& cpp_types_header)
{
   using namespace efl::eolian::grammar::attributes;

   std::vector<function_def> functions;
   Eina_Iterator *itr = eolian_declarations_get_by_file(fname.c_str());
   /* const */ Eolian_Declaration *decl;

   // Build list of functions with their parameters
   while(::eina_iterator_next(itr, reinterpret_cast<void**>(&decl)))
     {
        Eolian_Declaration_Type dt = eolian_declaration_type_get(decl);
        if (dt != EOLIAN_DECL_ALIAS)
          continue;

        const Eolian_Typedecl *tp = eolian_declaration_data_type_get(decl);
        if (!tp || eolian_typedecl_is_extern(tp))
          continue;

        if (::eolian_typedecl_type_get(tp) != EOLIAN_TYPEDECL_FUNCTION_POINTER)
          continue;

        const Eolian_Function *func = eolian_typedecl_function_pointer_get(tp);
        if (!func) return false;

        Eina_Iterator *param_itr = eolian_function_parameters_get(func);
        std::vector<parameter_def> params;

        /* const */ Eolian_Function_Parameter *param;
        while (::eina_iterator_next(param_itr, reinterpret_cast<void **>(&param)))
          {
             parameter_direction param_dir;
             switch (eolian_parameter_direction_get(param))
               {
                /* Note: Inverted on purpose, as the direction objects are
                 * passed is inverted (from C to C++ for function pointers).
                 * FIXME: This is probably not right in all cases. */
                case EOLIAN_IN_PARAM: param_dir = parameter_direction::out; break;
                case EOLIAN_INOUT_PARAM: param_dir = parameter_direction::inout; break;
                case EOLIAN_OUT_PARAM: param_dir = parameter_direction::in; break;
                default: return false;
               }

             const Eolian_Type *param_type_eolian = eolian_parameter_type_get(param);
             type_def param_type(param_type_eolian, opts.unit, EOLIAN_C_TYPE_PARAM);
             std::string param_name = eolian_parameter_name_get(param);
             std::string param_c_type = eolian_type_c_type_get(param_type_eolian, EOLIAN_C_TYPE_PARAM);
             parameter_def param_def(param_dir, param_type, param_name, param_c_type);
             params.push_back(std::move(param_def));
          }
        ::eina_iterator_free(param_itr);

        const Eolian_Type *ret_type_eolian = eolian_function_return_type_get(func, EOLIAN_FUNCTION_POINTER);

        type_def ret_type = void_;
        if (ret_type_eolian)
          ret_type = type_def(ret_type_eolian, opts.unit, EOLIAN_C_TYPE_RETURN);

        /*
        // List namespaces. Not used as function_wrapper lives in efl::eolian.
        std::vector<std::string> namespaces;
        Eina_Iterator *ns_itr = eolian_typedecl_namespaces_get(tp);
        char *ns;
        while (::eina_iterator_next(ns_itr, reinterpret_cast<void**>(&ns)))
          namespaces.push_back(std::string(ns));
        ::eina_iterator_free(ns_itr);
        */

        std::string name = eolian_function_name_get(func);
        std::string c_name = eolian_typedecl_full_name_get(tp);
        std::replace(c_name.begin(), c_name.end(), '.', '_');
        bool beta = eolian_function_is_beta(func);

        function_def def(ret_type, name, params, c_name, beta, false, true);
        functions.push_back(std::move(def));
     }
   ::eina_iterator_free(itr);

   if (functions.empty())
     return true;

   std::stringstream sink;

   sink.write("\n", 1);
   if (!efl::eolian::grammar::types_definition
       .generate(std::ostream_iterator<char>(sink),
                 functions, efl::eolian::grammar::context_null()))
     return false;

   cpp_types_header = sink.str();

   return true;
}

static void
run(options_type const& opts)
{
   if(!opts.main_header)
     {
       const Eolian_Class *klass = nullptr;
       char* dup = strdup(opts.in_files[0].c_str());
       char* base = basename(dup);
       std::string cpp_types_header;
       klass = ::eolian_class_get_by_file(nullptr, base);
       free(dup);
       if (klass)
         {
           if (!types_generate(base, opts, cpp_types_header) ||
               !generate(klass, opts, cpp_types_header))
             {
               EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
                 << "Error generating: " << ::eolian_class_name_get(klass)
                 << std::endl;
               assert(false && "error generating class");
             }
         }
       else
         {
           std::abort();
         }
     }
   else
     {
       std::set<std::string> headers;
       std::set<std::string> eo_files;

       for(auto&& name : opts.in_files)
         {
           Eolian_Unit const* unit = ::eolian_file_parse(name.c_str());
           if(!unit)
             {
               EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
                 << "Failed parsing: " << name << ".";
             }
           else
             {
               if(!opts.unit)
                 opts.unit = unit;
             }
           char* dup = strdup(name.c_str());
           char* base = basename(dup);
           Eolian_Class const* klass = ::eolian_class_get_by_file(unit, base);
           free(dup);
           if (klass)
             {
               std::string filename = eolian_class_file_get(klass);
               headers.insert(filename + std::string(".hh"));
               eo_files.insert(filename);
             }
         }

       using efl::eolian::grammar::header_include_directive;
       using efl::eolian::grammar::implementation_include_directive;

       auto main_header_grammar =
         *header_include_directive // sequence<string>
         << *implementation_include_directive // sequence<string>
         ;

       std::tuple<std::set<std::string>&, std::set<std::string>&> attributes{headers, eo_files};

       std::ofstream main_header;
       main_header.open(opts.out_file);
       if (!main_header.good())
         {
           EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
             << "Can't open output file: " << opts.out_file << std::endl;
           return;
         }
       
       main_header_grammar.generate(std::ostream_iterator<char>(main_header)
                                    , attributes, efl::eolian::grammar::context_null());
     }
}

static void
database_load(options_type const& opts)
{
   for (auto src : opts.include_dirs)
     {
        if (!::eolian_directory_scan(src.c_str()))
          {
             EINA_CXX_DOM_LOG_WARN(eolian_cxx::domain)
               << "Couldn't load eolian from '" << src << "'.";
          }
     }
   if (!::eolian_all_eot_files_parse())
     {
        EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
          << "Eolian failed parsing eot files";
        assert(false && "Error parsing eot files");
     }
   if (opts.in_files.empty())
     {
       EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
         << "No input file.";
       assert(false && "Error parsing input file");
     }
   if (!opts.main_header && !::eolian_file_parse(opts.in_files[0].c_str()))
     {
       EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain)
         << "Failed parsing: " << opts.in_files[0] << ".";
       assert(false && "Error parsing input file");
     }
}

} // namespace eolian_cxx {

static void
_print_version()
{
   std::cerr
     << "Eolian C++ Binding Generator (EFL "
     << PACKAGE_VERSION << ")" << std::endl;
}

static void
_usage(const char *progname)
{
   std::cerr
     << progname
     << " [options] [file.eo]" << std::endl
     << " A single input file must be provided (unless -a is specified)." << std::endl
     << "Options:" << std::endl
     << "  -a, --all               Generate bindings for all Eo classes." << std::endl
     << "  -c, --class <name>      The Eo class name to generate code for." << std::endl
     << "  -D, --out-dir <dir>     Output directory where generated code will be written." << std::endl
     << "  -I, --in <file/dir>     The source containing the .eo descriptions." << std::endl
     << "  -o, --out-file <file>   The output file name. [default: <classname>.eo.hh]" << std::endl
     << "  -n, --namespace <ns>    Wrap generated code in a namespace. [Eg: efl::ecore::file]" << std::endl
     << "  -r, --recurse           Recurse input directories loading .eo files." << std::endl
     << "  -v, --version           Print the version." << std::endl
     << "  -h, --help              Print this help." << std::endl;
   exit(EXIT_FAILURE);
}

static void
_assert_not_dup(std::string option, std::string value)
{
   if (value != "")
     {
        EINA_CXX_DOM_LOG_ERR(eolian_cxx::domain) <<
          "Option -" + option + " already set (" + value + ")";
     }
}

static eolian_cxx::options_type
opts_get(int argc, char **argv)
{
   eolian_cxx::options_type opts;

   const struct option long_options[] =
     {
       { "in",          required_argument, nullptr, 'I' },
       { "out-file",    required_argument, nullptr, 'o' },
       { "version",     no_argument,       nullptr, 'v' },
       { "help",        no_argument,       nullptr, 'h' },
       { "main-header", no_argument,       nullptr, 'm' },
       { nullptr,       0,                 nullptr,  0  }
     };
   const char* options = "I:D:o:c::marvh";

   int c, idx;
   while ( (c = getopt_long(argc, argv, options, long_options, &idx)) != -1)
     {
        if (c == 'I')
          {
             opts.include_dirs.push_back(optarg);
          }
        else if (c == 'o')
          {
             _assert_not_dup("o", opts.out_file);
             opts.out_file = optarg;
          }
        else if (c == 'h')
          {
             _usage(argv[0]);
          }
        else if(c == 'm')
          {
            opts.main_header = true;
          }
        else if (c == 'v')
          {
             _print_version();
             if (argc == 2) exit(EXIT_SUCCESS);
          }
     }
   if (optind != argc)
     {
       for(int i = optind; i != argc; ++i)
         opts.in_files.push_back(argv[i]);
     }

   if (!eolian_cxx::opts_check(opts))
     {
        _usage(argv[0]);
        assert(false && "Wrong options passed in command-line");
     }

   return opts;
}

int main(int argc, char **argv)
{
   try
     {
        efl::eina::eina_init eina_init;
        efl::eolian::eolian_init eolian_init;
        eolian_cxx::options_type opts = opts_get(argc, argv);
        eolian_cxx::database_load(opts);
        eolian_cxx::run(opts);
     }
   catch(std::exception const& e)
     {
       std::cerr << "EOLCXX: Eolian C++ failed generation for the following reason: " << e.what() << std::endl;
       std::cout << "EOLCXX: Eolian C++ failed generation for the following reason: " << e.what() << std::endl;
       return -1;
     }
   return 0;
}
