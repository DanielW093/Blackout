#include <irrlicht.h>

using namespace irr;

using namespace core;
using namespace scene;
using namespace video;
using namespace io;
using namespace gui;
struct SAppContext
{
    IrrlichtDevice *device;
    s32             counter;
    IGUIListBox*    listbox;
};

enum
{
    GUI_ID_QUIT_BUTTON = 101,
	GUI_ID_NEW_GAME_BUTTON,
	GUI_ID_SETTINGS_BUTTON,
    GUI_ID_NEW_WINDOW_BUTTON,
    GUI_ID_FILE_OPEN_BUTTON,
    GUI_ID_TRANSPARENCY_SCROLL_BAR,
	GUI_ID_SAVE_AND_QUIT_BUTTON
};

class EventReceiver : public IEventReceiver
{
public:
    EventReceiver(SAppContext & context) : Context(context) 
	{
		gameStarted = false;
		quitToMenu = false;
		for (u32 i=0; i<KEY_KEY_CODES_COUNT; ++i)
			KeyIsDown[i] = false;
	}

	void setSkinTransparency(s32 alpha, IGUISkin * skin)
	{
		for (s32 i=0; i<EGDC_COUNT ; ++i)
		{
		    SColor col = skin->getColor((EGUI_DEFAULT_COLOR)i);
		    col.setAlpha(alpha);
			skin->setColor((EGUI_DEFAULT_COLOR)i, col);
		}
	}

    virtual bool OnEvent(const SEvent& event)
    {
		if (event.EventType == irr::EET_KEY_INPUT_EVENT)
			KeyIsDown[event.KeyInput.Key] = event.KeyInput.PressedDown;
        if (event.EventType == EET_GUI_EVENT)
        {
            s32 id = event.GUIEvent.Caller->getID();
            IGUIEnvironment* env = Context.device->getGUIEnvironment();
			
            switch(event.GUIEvent.EventType)
            {
            case EGET_SCROLL_BAR_CHANGED:
                if (id == GUI_ID_TRANSPARENCY_SCROLL_BAR)
                {
                    s32 pos = ((IGUIScrollBar*)event.GUIEvent.Caller)->getPos();
                    setSkinTransparency(pos, env->getSkin());
                }
                break;
            case EGET_BUTTON_CLICKED:
                switch(id)
                {
				case GUI_ID_SAVE_AND_QUIT_BUTTON:
					quitToMenu = true;
					return true;

                case GUI_ID_QUIT_BUTTON:
                    Context.device->closeDevice();
                    return true;
				
				case GUI_ID_NEW_GAME_BUTTON:

					gameStarted = true;
					return true;
				case GUI_ID_SETTINGS_BUTTON:

					return true;
                case GUI_ID_NEW_WINDOW_BUTTON:
                    {
                    Context.listbox->addItem(L"Window created");
                    Context.counter += 30;
                    if (Context.counter > 200)
                        Context.counter = 0;

                    IGUIWindow* window = env->addWindow(
                        rect<s32>(100 + Context.counter, 100 + Context.counter, 300 + Context.counter, 200 + Context.counter),
                        false, // modal?
                        L"Test window");

                    env->addStaticText(L"Please close me",
                        rect<s32>(35,35,140,50),
                        true, // border?
                        false, // wordwrap?
                        window);
                    }
                    return true;

                case GUI_ID_FILE_OPEN_BUTTON:
                    Context.listbox->addItem(L"File open");
                    // There are some options for the file open dialog
                    // We set the title, make it a modal window, and make sure
                    // that the working directory is restored after the dialog
                    // is finished.
                    env->addFileOpenDialog(L"Please choose a file.", true, 0, -1, true);
                    return true;

                default:
                    return false;
                }
                break;

            case EGET_FILE_SELECTED:
                {
                    // show the model filename, selected in the file dialog
                    IGUIFileOpenDialog* dialog =
                        (IGUIFileOpenDialog*)event.GUIEvent.Caller;
                    Context.listbox->addItem(dialog->getFileName());
                }
                break;

            default:
                break;
            }
        }

        return false;
    }

	virtual bool IsKeyDown(EKEY_CODE keyCode) const
	{
		return KeyIsDown[keyCode];
	}

	bool getGameStarted()
	{
		bool retVal;

		retVal = gameStarted;

		gameStarted = false;

		return retVal;
	}

	bool getQuitToMenu()
	{
		bool retVal;

		retVal = quitToMenu;

		quitToMenu = false;
		
		return retVal;
	}

private:
    SAppContext & Context;
	bool KeyIsDown[KEY_KEY_CODES_COUNT];
	bool gameStarted;
	bool quitToMenu;
};