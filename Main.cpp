#include <SDK/foobar2000.h>
#include <atlbase.h>
#include <atlwin.h>
#include <atlapp.h>
#include <atlctrls.h>
#include "resource.h"
#include <boost/spirit/home/qi.hpp>
#include <vector>

struct SeekBox : CSimpleDialog<IDD_SEEKBOX>
{
    SeekBox()
      : valid(false)
    {
    }

    BEGIN_MSG_MAP(SeekBox)
    MESSAGE_HANDLER(WM_COMMAND, onCommand);
    END_MSG_MAP()

    LRESULT onCommand(UINT msg, WPARAM wparam, LPARAM lparam, BOOL& handled)
    {
        switch (wparam) {
            case 1: {
                CEdit box = (HWND)GetDlgItem(IDC_EDIT);
                int cch = box.LineLength(0);
                std::vector<wchar_t> buf(cch + 1, L'\0');
                box.GetLine(0, &buf[0], cch);

                using namespace boost::spirit::qi;
                std::vector<int> parts;
                parse(buf.begin(), buf.end(), uint_ % (lit(':') | '.' | ' '), parts);

                switch (parts.size()) {
                    case 1:
                        s = parts[0];
                        m = s / 60;
                        h = m / 60;
                        m %= 60;
                        s %= 60;
                        valid = true;
                        break;
                    case 2:
                        s = parts[1];
                        m = parts[0];
                        h = m / 60;
                        m %= 60;
                        valid = s < 60;
                        break;
                    case 3:
                        s = parts[2];
                        m = parts[1];
                        h = parts[0];
                        valid = s < 60 && m < 60;
                        break;
                }

                if (valid) {
                    EndDialog(*this, TRUE);
                    return TRUE;
                }
                EndDialog(*this, FALSE);
                break;
            }
            case 2: {
                EndDialog(*this, FALSE);
                break;
            }
        }
        handled = FALSE;
        return FALSE;
    }

    int h, m, s;
    bool valid;
};

struct SeekBoxCB : main_thread_callback
{
    virtual void callback_run()
    {
        SeekBox sb;
        INT_PTR ret = sb.DoModal(core_api::get_main_window());
        if (ret == TRUE) {
            static_api_ptr_t<playback_control> pc;
            pc->playback_seek(sb.h * 3600 + sb.m * 60 + sb.s);
        }
    }
};

struct SeekMenu : mainmenu_commands
{
    t_uint32 get_command_count() { return 1; }
    GUID get_command(t_uint32 p_index) { return s_guid; }
    void get_name(t_uint32 p_index, pfc::string_base& p_out) { p_out = "Seek to Time..."; }
    bool get_description(t_uint32 p_index, pfc::string_base& p_out)
    {
        p_out = "Opens a modal dialog with an edit box to seek within the playing track.";
        return true;
    }
    GUID get_parent() { return mainmenu_groups::playback_controls; }
    void execute(t_uint32, service_ptr)
    {
        static_api_ptr_t<main_thread_callback_manager> mtcm;
        mtcm->add_callback(new service_impl_t<SeekBoxCB>);
    }

    static GUID const s_guid;
};

// {040E59E3-442F-4f41-B18F-8CE9692E7678}
GUID const SeekMenu::s_guid = { 0x40e59e3, 0x442f, 0x4f41, { 0xb1, 0x8f, 0x8c, 0xe9, 0x69, 0x2e, 0x76, 0x78 } };

static mainmenu_commands_factory_t<SeekMenu> g_asdf;

DECLARE_COMPONENT_VERSION("Seek box", "0.0.3", "zao")
VALIDATE_COMPONENT_FILENAME("foo_seek_box.dll")