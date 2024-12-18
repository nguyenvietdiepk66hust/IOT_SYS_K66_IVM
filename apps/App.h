/**
@file
Application Template.
*/
/*
  
  Library:: 

  This version is

  Copyright:: 2021 nguyentrinhtuan1996@gmail.com
*/
#ifndef _Application_
#define _Application_
/* _____PROJECT INCLUDES____________________________________________________ */

#include "Arduino.h"
/* _____GLOBAL VARIABLES_____________________________________________________ */
bool      Serial_Hardware_Port_Is_Opened = 0;
unsigned long  Serial_Hardware_Port_Baudrate = 115200;
/* _____DEFINITIONS__________________________________________________________ */
enum App_User_Mode
{
  APP_USER_MODE_NORMAL,
  APP_USER_MODE_DEBUG
};
enum App_State
{
  APP_STATE_PENDING,
  APP_STATE_STARTING,
  APP_STATE_READY,
  APP_STATE_EXECUTING,
  APP_STATE_SUSPENDING,
  APP_STATE_ENDING,
  APP_STATE_ERROR
};
enum App_Run_Mode
{
  APP_RUN_MODE_MANUAL,
  APP_RUN_MODE_AUTO
};
/* _____CLASS DEFINITION_____________________________________________________ */
/**
 * This Application class is base class for implement another class by inheritance 
 */
class Application
{
public:
  Application();
  
  void      Run_Application(bool autoRun);
  // 
  char*     Name_Application = (char*)"Application Name";
  uint16_t  ID_Application = 0;

  // user can existing state of app by change Application_State into below state
  
  //  the tasks that User want to process by the task
  uint8_t Application_State = APP_STATE_PENDING;

  
  // enter task debug
  void Debug();
  // exit task debug
  void Debug_Exit();
  // 
  uint8_t User_Mode =  APP_USER_MODE_NORMAL;
  // this var will be set to 1 whenever app done a state execution 
  bool Step_Forward = 0;
protected:
  void (*_Pend_User)();
  void (*_Start_User)();
  void (*_Restart_User)();
  void (*_Execute_User)();
  void (*_Suspend_User)();
  void (*_Resume_User)();
  void (*_End_User)();
  
  uint8_t   State_Application();
  char*     State_Application_String();
  void      Information();  
private:
  bool Run_Mode = Run_Mode_Auto;// the executing task will run APP_RUN_MODE_AUTOally

  static const bool Run_Mode_Auto = 1;
  static const bool Run_Mode_Manual = 0;

  //  Tasks default of application
  void Pend(void);
  void Start(void);
  void Restart(void);
  void Ready(void);
  void Execute(void);
  void Suspend(void);
  void Resume(void);
  void End(void);
};

/**
 */
Application::Application(void)
{
  _Pend_User    = 0;
  _Start_User   = 0;
  _Restart_User = 0;
  _Execute_User = 0;
  _Suspend_User = 0;
  _Resume_User  = 0;
  _End_User     = 0;
}
/**
 * Enter debug mode
 */
void Application::Debug()
{
  User_Mode =  APP_USER_MODE_DEBUG;
}
/**
 * exit debug mode
 */
void Application::Debug_Exit()
{
  User_Mode = APP_USER_MODE_NORMAL;
}

/**
 * The main task of application, need to bee call regularly for attack like a 
 * task in freertos. There are 7 state of an application. Pending state is the state of 
 * application is waiting to start. Starting is initting the prerequisite condition to
 * execute. To be Ready for executing is need for executing the task. Executing state is
 * main state to handle the task of application. Suspend state will freeze the application
 * but will not remote the parameter. Application will finish all task and save all
 * parameter, strop and remove all parameter. Error is the indicator for user when the 
 * application is executing wrongly.
 * @param autoRun  APP_RUN_MODE_AUTO for auto execute and APP_RUN_MODE_MANUAL for wait for execute manually by user
 */
void Application::Run_Application(bool autoRun = APP_RUN_MODE_AUTO)
{
  switch (Application_State)
  {
    // pending to start the task
    case APP_STATE_PENDING:
      Run_Mode = autoRun;
      Pend();
      break;
    // starting the task
    case APP_STATE_STARTING:
      Start();
      break;

    // task is read, if the task is set by aut running, task will be auto executing, unless it wil be executed APP_RUN_MODE_MANNALly.
    case APP_STATE_READY:
      Ready();
      break;

    //the task is executing 
    case APP_STATE_EXECUTING:
      Execute();
      break;

    // the task is been suspending
    case APP_STATE_SUSPENDING:
      Suspend();
      break;

    // the task is been ending
    case APP_STATE_ENDING:
      End();
      break;

    // default
    default:

      break;
  }
}
/**
 * This function will return a unsigned integer of application state at the present 
 * @return Application_State the
 */
uint8_t Application::State_Application()
{
  return Application_State;
}
char* Application::State_Application_String()
{
  switch (Application_State)
  {
  case APP_STATE_PENDING:
    return (char*)"Pending State";
    break;
  case APP_STATE_STARTING:
    return (char*)"Starting State";
    break; 
  case APP_STATE_READY:
    return (char*)"Ready State";
    break;
  case APP_STATE_EXECUTING:
    return (char*)"Executing State";
    break;
  case APP_STATE_SUSPENDING:
    return (char*)"Suspending State";
    break;
  case APP_STATE_ENDING:
    return (char*)"Ending State";
    break;
  default:
    return (char*)"Error state";
    break;
  }
}
/**
 *  Pend to start is the first task of this application it will do prerequisite
 *  condition. In the debug mode, task will send Information of application to 
 * terminal to start the application.
 * @param (*_Pend_User)() this function pointer will be define by user
 * @param User_Mode will be specified by user
 */
void Application::Pend()
{
  // the user function
  (*_Pend_User)();
  if (User_Mode == APP_USER_MODE_DEBUG) 
  {
    // if the serial port is not open, thus open it
    if(!Serial_Hardware_Port_Is_Opened) 
    {
      Serial.begin(Serial_Hardware_Port_Baudrate);
      Serial_Hardware_Port_Is_Opened = 1;
    }
    Information();
  }
  Application_State = APP_STATE_STARTING;
  Step_Forward = 1;
}
/**
 *  Pend to start is the first task of this application it will do prerequisite
 *  condition. In the debug mode, task will send Information of application to 
 * terminal to start the application.
 * @param (*_Start_User)() this function pointer will be define by user
 * @param User_Mode will be specified by user
 */
void Application::Start()
{
  if (User_Mode == APP_USER_MODE_DEBUG) Information();
  // the user function
  (*_Start_User)();
  Application_State = APP_STATE_READY;
  Step_Forward = 1;
}
/**
 *  Pend to start is the first task of this application it will do prerequisite
 *  condition. In the debug mode, task will send Information of application to 
 * terminal to start the application.
 * @param (*_Start_User)() this function pointer will be define by user
 * @param User_Mode will be specified by user
 */
void Application::Ready()
{
  if (User_Mode == APP_USER_MODE_DEBUG) Information();
  // the user function
  if(Run_Mode) Application_State = APP_STATE_EXECUTING;
  Step_Forward = 1;
}
/**
 *  Pend to start is the first task of this application it will do prerequisite
 *  condition. In the debug mode, task will send Information of application to 
 * terminal to start the application.
 * @param (*_Restart_User)() this function pointer will be define by user
 * @param User_Mode will be specified by user
 */
/**
 *  Pend to start is the first task of this application it will do prerequisite
 *  condition. In the debug mode, task will send Information of application to 
 * terminal to start the application.
 * @param (*_Execute_User)() this function pointer will be define by user
 * @param User_Mode will be specified by user
 */
void Application::Execute()
{
  if (User_Mode == APP_USER_MODE_DEBUG) Information();
  // the user function
  (*_Execute_User)();
  Step_Forward = 1;
}
/**
 *  Pend to start is the first task of this application it will do prerequisite
 *  condition. In the debug mode, task will send Information of application to 
 * terminal to start the application.
 * @param (*_Suspend_User)() this function pointer will be define by user
 * @param User_Mode will be specified by user
 */
void Application::Suspend()
{
  if (User_Mode == APP_USER_MODE_DEBUG) Information();
  // the user function
  (*_Suspend_User)();
  Step_Forward = 1;
}
/**
 *  Pend to start is the first task of this application it will do prerequisite
 *  condition. In the debug mode, task will send Information of application to 
 * terminal to start the application.
 * @param (*_Resume_User)() this function pointer will be define by user
 * @param User_Mode will be specified by user
 */
void Application::Resume()
{
  
  if (User_Mode == APP_USER_MODE_DEBUG) Information();
  // the user function
  (*_Resume_User)();
  Application_State = APP_STATE_EXECUTING;
  Step_Forward = 1;
}
/**
 *  Pend to start is the first task of this application it will do prerequisite
 *  condition. In the debug mode, task will send Information of application to 
 * terminal to start the application.
 * @param (*_End_User)() this function pointer will be define by user
 * @param User_Mode will be specified by user
 */
void Application::End()
{
  if (User_Mode == APP_USER_MODE_DEBUG) Information();
  // the user function
  (*_End_User)();
  Step_Forward = 1;
}

/**
 * Print the the Information of application. These Informations are ID, name,
 *  state of application and the remaining free head size. The Information will be 
 * write to the Serial terminal
 */
void Application::Information()
{
  Serial.printf_P("ID: %d -- ",ID_Application);
  Serial.printf_P(Name_Application);
  Serial.printf_P(" : ");
  Serial.printf_P(State_Application_String());
  Serial.printf_P(" -- Free heap size %d.\n",ESP.getFreeHeap());

}
#endif