// keyboard input
#include "pch.h"

// ---------------------------------------------------------------------------

static void con_function_key(int vkey, int ctrl)
{
    switch(vkey)
    {
        case VK_F1:
            con_update_registers();
            con.update |= CON_UPDATE_REGS;
            con_refresh();
            break;
        case VK_F2:
            con_change_focus(WDATA);    // Data
            break;
        case VK_F3:
            con_change_focus(WDISA);    // Disa
            break;
        case VK_F4:
            con_change_focus(WCONSOLE); // Console (roll)
            break;
        case VK_F5:
            if (Gekko::Gekko)
            {
                if (Gekko::Gekko->IsRunning())
                {
                    con_break();
                }
                else
                {
                    Gekko::Gekko->Run();
                }
            }
            break;
        case VK_F6:
            // Switch Registers View
            if(ctrl & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED))
            {
                if(wind.regmode == REGMOD_GPR)
                {
                    wind.regmode = (REGWNDMODE)(REGMOD_MAX - 1);
                    memset(con.buf, 0, sizeof(CHAR_INFO) * CON_WIDTH * wind.regs_h);
                    con.update |= CON_UPDATE_REGS;
                    return;
                }
                else
                {
                    // back enum
                    wind.regmode = (REGWNDMODE) ((int)wind.regmode - 1);
                    memset(con.buf, 0, sizeof(CHAR_INFO) * CON_WIDTH * wind.regs_h);
                    con.update |= CON_UPDATE_REGS;
                }
            }
            else
            {
                if(wind.regmode == REGMOD_MAX - 1)
                {
                    wind.regmode = (REGWNDMODE)0;
                    memset(con.buf, 0, sizeof(CHAR_INFO) * CON_WIDTH * wind.regs_h);
                    con.update |= CON_UPDATE_REGS;
                    return;
                }
                else
                {
                    // forward enum
                    wind.regmode = (REGWNDMODE)((int)wind.regmode + 1);
                    memset(con.buf, 0, sizeof(CHAR_INFO) * CON_WIDTH * wind.regs_h);
                    con.update |= CON_UPDATE_REGS;
                }
            }
            break;
        case VK_F9:     // Toggle Breakpoint
            Gekko::Gekko->ToggleBreakpoint(con.disa_cursor);
            con.update |= CON_UPDATE_DISA;
            break;
        case VK_F10:    // Step Over
            Gekko::Gekko->AddOneShotBreakpoint(Gekko::Gekko->regs.pc + 4);
            Gekko::Gekko->Run();
            break;
        case VK_F11:    // Step Into
            con_step_into();
            break;
        case VK_F12:    // Skip
            Gekko::Gekko->regs.pc += 4;
            con.update |= CON_UPDATE_DISA;
            DBReport2(DbgChannel::CPU, "skipped!\n");
            break;
    }
}

// searching for beginning of each word
static void search_left()
{
    roll.editlen = (int)strlen(roll.editline);

    // check editpos!=0 for possibility check [editpos-1]
    if((roll.editlen == 0) || (roll.editpos == 0)) return;

    // we at beginning of the word? if so, move to the space
    if((roll.editline[roll.editpos] != 0x20) && (roll.editline[roll.editpos - 1] == 0x20)) roll.editpos--;

    // note: no need check editpos!=0 here
    if(roll.editline[roll.editpos] == 0x20)
    {   // are we at space?
        //search for first non-space
        while((roll.editpos != 0) && (roll.editline[roll.editpos] == 0x20)) roll.editpos--;
        if(!roll.editpos) return;
    }

    // we at last char, search for first space now
    while(roll.editpos != 0)
    {
        // sure, editpos!=0  here, coz while check it first
        if(roll.editline[roll.editpos - 1] == 0x20) return;
        roll.editpos--;
    }
}

static void search_right()
{
    roll.editlen = (int)strlen(roll.editline);
    if((roll.editlen == 0) || (roll.editpos == roll.editlen)) return;
    if(roll.editline[roll.editpos] != 0x20)
    {
        while(roll.editline[roll.editpos] != 0x20)
        {
            if(roll.editpos == roll.editlen) return;
            roll.editpos++;
        }
    }

    while((roll.editpos != roll.editlen) && (roll.editline[roll.editpos] == 0x20)) roll.editpos++;
}

#define endl    ( line[p] == 0 )
#define space   ( line[p] == 0x20 )
#define quot    ( line[p] == '\'' )
#define dquot   ( line[p] == '\"' )

void con_tokenizing(char *line)
{
    int p, start, end;
    p = roll.tokencount = start = end = 0;
    memset(roll.tokens, 0, sizeof(roll.tokens));

    // while not end line
    while(!endl)
    {
        // skip space first, if any
        while(space) p++;
        if(!endl && (quot || dquot))
        {   // quotation, need special case
            p++;
            start = p;
            while(1)
            {
                if(endl)
                {
                    con_print(ConColor::BRED, "Open quotation\n");
                    return;
                }

                if(quot || dquot)
                {
                    end = p;
                    p++;
                    break;
                }
                else p++;
            }

            if(roll.tokencount >= CON_TOKENCNT)
            {
                con_print(ConColor::BRED, "Too many args (>%i) or need quotes\n", CON_TOKENCNT);
                return;
            }

            strncpy_s (roll.tokens[roll.tokencount], sizeof(roll.tokens[roll.tokencount]), line + start, end - start);

            // make lower only first token
            if(roll.tokencount == 0) _strlwr_s(roll.tokens[roll.tokencount], sizeof(roll.tokens[roll.tokencount]));
            roll.tokencount++;
        }
        else if(!endl)
        {
            start = p;
            while(1)
            {
                if(endl || space || quot || dquot)
                {
                    end = p;
                    break;
                }

                p++;
            }

            if(roll.tokencount >= CON_TOKENCNT)
            {
                con_print(ConColor::BRED, "Too many args (>%i) or need quotes\n", CON_TOKENCNT);
                return;
            }

            strncpy_s (roll.tokens[roll.tokencount], sizeof(roll.tokens[roll.tokencount]), line + start, end - start);
            roll.tokencount++;
        }
    }
}

#undef space
#undef quot
#undef dquot
#undef endl

static int testempty(char *str)
{
    int i, len = (int)strlen(str);

    for(i=0; i<len; i++)
    {
        if(str[i] != 0x20) return 0;
    }

    return 1;
}

static void con_roll_edit_key(char ascii, int vkey, int ctrl)
{
    std::vector<std::string> args;
    if(ascii >= 0x20 && ascii < 256)
    {
        roll.editlen = (int)strlen(roll.editline);
        if(roll.editlen >= 77) return;
        else
        {
            if(roll.editpos == roll.editlen)
            {
                roll.editline[roll.editpos++] = ascii;
                roll.editline[roll.editpos] = 0;
            }
            else
            {
                memmove(
                    roll.editline + roll.editpos + 1, 
                    roll.editline + roll.editpos, 
                    roll.editlen - roll.editpos + 1
                );
                roll.editline[roll.editpos++] = ascii;
            }

            con_update(CON_UPDATE_EDIT);
        }
    }
    else
    {
        switch(vkey)
        {
            case VK_RETURN:
                roll.editlen = (int)strlen(roll.editline);
                if(!roll.editlen) return;
                if(testempty(roll.editline)) return;
                strcpy_s (roll.history[roll.historypos], sizeof(roll.history[roll.historypos]), roll.editline);
                roll.historypos++;
                if(roll.historypos > 255) roll.historypos = 0;
                roll.historycur = roll.historypos;
                con_print(ConColor::NORM, ": %s", roll.editline);
                con_tokenizing(roll.editline);
                roll.editpos = roll.editlen = roll.editline[0] = 0;
                con_update(CON_UPDATE_EDIT | CON_UPDATE_MSGS);
                for (int i = 0; i < roll.tokencount; i++)
                {
                    args.push_back(roll.tokens[i]);
                }
                con_command(args);
                break;
            case VK_UP:
                if(!roll.autoscroll)
                {
                    roll.viewpos --;
                    if(roll.viewpos < 0) roll.viewpos = 0;
                    con_update(CON_UPDATE_MSGS);
                }
                else
                {
                    if(--roll.historycur < 0) roll.historycur = 0;
                    else
                    {
                        strcpy_s (roll.editline, sizeof(roll.editline), roll.history[roll.historycur]);
                        roll.editpos = roll.editlen = (int)strlen(roll.editline);
                        con_update(CON_UPDATE_EDIT);
                    }
                }
                break;
            case VK_DOWN:
                if(!roll.autoscroll)
                {
                    roll.viewpos ++;
                    if(roll.viewpos >= roll.rollpos)
                    {
                        roll.viewpos = roll.rollpos;
                        con_set_autoscroll(TRUE);
                    }
                    con_update(CON_UPDATE_MSGS);
                }
                else
                {
                    if(++roll.historycur >= roll.historypos)
                    {
                        roll.historycur = roll.historypos;
                        roll.editpos = roll.editlen = roll.editline[0] = 0;
                        con_update(CON_UPDATE_EDIT);
                    }
                    else
                    {
                        strcpy_s (roll.editline, sizeof(roll.editline), roll.history[roll.historycur]);
                        roll.editpos = roll.editlen = (int)strlen(roll.editline);
                        con_update(CON_UPDATE_EDIT);
                    }
                }
                break;
            case VK_HOME:
                if(!roll.autoscroll)
                {
                    roll.viewpos = 0;
                    con_update(CON_UPDATE_MSGS);
                }
                else
                {
                    roll.editpos = 0;
                    con_update(CON_UPDATE_EDIT);
                }
                break;
            case VK_END:
                if(!roll.autoscroll)
                {
                    roll.viewpos = roll.rollpos;
                    con_update(CON_UPDATE_MSGS);
                }
                else
                {
                    roll.editpos = (int)strlen(roll.editline);
                    con_update(CON_UPDATE_EDIT);
                }
                break;
            case VK_LEFT:
                if(ctrl & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) search_left();
                else if(roll.editpos) roll.editpos--;
                con_update(CON_UPDATE_EDIT);
                break;
            case VK_RIGHT:
                roll.editlen = (int)strlen(roll.editline);
                if(roll.editlen && (roll.editpos != roll.editlen))
                {
                    if(ctrl & (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)) search_right();
                    else if(roll.editpos < roll.editlen) roll.editpos++;
                    con_update(CON_UPDATE_EDIT);
                }
                break;
            case VK_BACK:
                if(roll.editpos)
                {
                    roll.editlen = (int)strlen(roll.editline);
                    if(roll.editlen == roll.editpos) roll.editline[--roll.editpos] = 0;
                    else memmove(
                        roll.editline + roll.editpos - 1, 
                        roll.editline + roll.editpos, 
                        roll.editlen - roll.editpos + 1
                    ), roll.editpos--;
                    con_update(CON_UPDATE_EDIT);
                }
                break;
            case VK_ESCAPE:
                if(!roll.autoscroll)
                {
                    con_set_autoscroll(TRUE);
                    con_update(CON_UPDATE_MSGS);
                }
                else
                {
                    roll.historycur = roll.historypos;
                    roll.editpos = roll.editlen = roll.editline[0] = 0;
                    con_update(CON_UPDATE_EDIT);
                }
                break;
            case VK_PRIOR:
                if(roll.autoscroll)
                {
                    con_set_autoscroll(FALSE);
                    roll.viewpos = roll.rollpos;
                }

                roll.viewpos -= 8;
                if(roll.viewpos < 0) roll.viewpos = 0;
                con_update(CON_UPDATE_MSGS);
                break;
            case VK_NEXT:
                if(roll.autoscroll)
                {
                    con_set_autoscroll(FALSE);
                    roll.viewpos = roll.rollpos;
                }

                roll.viewpos += 8;
                if(roll.viewpos >= roll.rollpos)
                {
                    roll.viewpos = roll.rollpos;
                    con_set_autoscroll(TRUE);
                }
                con_update(CON_UPDATE_MSGS);
                break;
        }
    }
}

static void con_virtualkey(char ascii, int vkey, int ctrl)
{
    // functions F1..F12 and control
    if((vkey >= VK_F1) && (vkey <= VK_F12))
    {
        con_function_key(vkey, ctrl);
    }

    // switch focus to input line if symbol key
    if(vkey == 0x8 || (ascii >= 0x20 && ascii < 256))
    {
        con_change_focus(WCONSOLE);
    }

    switch(wind.focus)
    {
        case WREGS:
            break;
        case WDATA:
            con_data_key(ascii, vkey, ctrl);
            break;
        case WDISA:
            con_disa_key(ascii, vkey, ctrl);
            break;
        case WCONSOLE:
            con_roll_edit_key(ascii, vkey, ctrl);
            break;
    }
}

void con_read_input(int peek)
{
    INPUT_RECORD    record;
    DWORD           count;

    if(con.active == FALSE) return;

    if(peek)
    {
        PeekConsoleInput(con.input, &record, 1, &count);
        if(!count) return;
    }

    ReadConsoleInput(con.input, &record, 1, &count);
    if(!count) return;

    if(record.EventType == KEY_EVENT && record.Event.KeyEvent.bKeyDown)
    {
        char    ascii = record.Event.KeyEvent.uChar.AsciiChar;
        int     vcode = record.Event.KeyEvent.wVirtualKeyCode;
        int     ctrl = record.Event.KeyEvent.dwControlKeyState;
        con_virtualkey(ascii, vcode, ctrl);
    }
}
