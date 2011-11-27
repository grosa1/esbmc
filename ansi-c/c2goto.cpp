#include <ostream>
#include <fstream>

#include <goto-programs/goto_convert_functions.h>
#include <goto-programs/write_goto_binary.h>
#include <langapi/language_ui.h>
#include <langapi/mode.h>
#include <ui_message.h>
#include <parseoptions.h>
#include <util/config.h>

class c2goto_parseopt : public parseoptions_baset, public language_uit
{
  public:
  c2goto_parseopt(int argc, const char **argv):
    parseoptions_baset("(16)(32)(64)(output):(no-lock-check)", argc, argv),
    language_uit(cmdline)
  {
  }

  int doit() {
    goto_functionst goto_functions;

    config.set(cmdline);

    if (!cmdline.isset("output")) {
      std::cerr << "Must set output file" << std::endl;
      return 1;
    }

    if (parse()) return 1;
    if (typecheck()) return 1;

    std::ofstream out(cmdline.getval("output"));

    if (write_goto_binary(out, context, goto_functions)) {
      std::cerr << "Failed to write C library to binary obj" << std::endl;;
      return 1;
    }

    return 0;
  }
};

int main(int argc, const char **argv)
{
  c2goto_parseopt parseopt(argc, argv);
  return parseopt.main();
}

const mode_table_et mode_table[] =
{
  LANGAPI_HAVE_MODE_C,
  LANGAPI_HAVE_MODE_END
};
