#ifndef _PLCEMU_H_
#define _PLCEMU_H_
/**
 *@file plcemu.h
 *@brief main header
*/

#define PRINTABLE_VERSION 2.0
//analog I/O

#define TERMLEN 80
#define TERMHGT 24

#define KEY_TAB		9
/*
typedef enum{
    MAP_ROOT,
    MAP_USPACE,
    MAP_COMEDI,
    MAP_COMEDI_SUBDEV,
    MAP_SIM,
    MAP_VARIABLE,
    N_MAPPINGS    
}CONFIG_MAPPINGS;

typedef enum{
    USPACE_BASE,
    USPACE_WR,
    USPACE_RD,
    N_USPACE_VARS
}USPACE_VARS;

typedef enum{
    SUBDEV_IN,
    SUBDEV_OUT,
    SUBDEV_ADC,
    SUBDEV_DAC,
    N_SUBDEV_VARS
}SUBDEV_VARS;

typedef enum{
    COMEDI_FILE,
    COMEDI_SUBDEV,
    N_COMEDI_VARS
}COMEDI_VARS; 

typedef enum {
    SIM_INPUT,
    SIM_OUTPUT,
    N_SIM_VARS
}SIM_VARS;

typedef enum{
    CONFIG_STEP,
    CONFIG_PIPE,
    CONFIG_HW,
    CONFIG_USPACE,
    CONFIG_COMEDI,
    CONFIG_SIM,
    
     //(runtime updatable) sequences,
    CONFIG_PROGRAM,
    CONFIG_AI,
    CONFIG_AQ,
    CONFIG_DI,
    CONFIG_DQ,
    CONFIG_MVAR,
    CONFIG_MREG,
    CONFIG_TIMER,
    CONFIG_PULSE, 
    N_CONFIG_VARIABLES
} CONFIG_VARIABLES;

typedef enum{
    CLI_COM,
    CLI_PROGRAM = CONFIG_PROGRAM,
    CLI_AI,
    CLI_AQ,
    CLI_DI,
    CLI_DQ,
    CLI_MVAR,
    CLI_MREG,
    CLI_TIMER,
    CLI_PULSE, 
    N_CLI_ARGS
} CLI_ARGS;
*/
//int plc_load_file(const char * path);
//int plc_save_file(const char * path);
#endif //_PLCEMU_H_
