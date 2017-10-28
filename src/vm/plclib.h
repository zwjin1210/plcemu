#ifndef _PLCLIB_H_
#define _PLCLIB_H_
/**
 *@file plclib.h
 *@brief PLC core virtual machine
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <inttypes.h>

#define MILLION 1000000
#define THOUSAND 1000

#define MAXRUNG 256 
#define MAXBUF 256 
#define MAXSTR	1024
#define MEDSTR	256
#define SMALLSTR 128
#define SMALLBUF 64
#define TINYBUF 24
#define TINYSTR 32
#define COMMLEN 16

#define LONG_BYTES 8

#define FLOAT_PRECISION 0.000001
#define ASCIISTART 0x30

typedef enum{
    ST_STOPPED,
    ST_RUNNING
}STATUSES;

typedef enum{
    N_ERR = -10, ///ERROR CODES are negative
    ERR_OVFLOW,
    ERR_TIMEOUT,
    ERR_BADOPERATOR,
    ERR_BADCOIL,
    ERR_BADINDEX,
    ERR_BADOPERAND,
    ERR_BADFILE,
    ERR_BADCHAR,
    ERR_BADPROG,
}ERRORCODES;

typedef enum{
    IE_PLC,
    IE_BADOPERATOR,
    IE_BADCOIL,
    IE_BADINDEX,
    IE_BADOPERAND,
    IE_BADFILE,
    IE_BADCHAR,
    N_IE 
} IL_ERRORCODES;

typedef enum{
    LANG_LD,
    LANG_IL,
    LANG_ST
}LANGUAGES;

typedef enum{///boolean function blocks supported
    BOOL_DI,        ///digital input
    BOOL_DQ,        ///digital output
    BOOL_COUNTER,   ///pulse of counter
    BOOL_TIMER,     ///output of timer
    BOOL_BLINKER,   ///output of blinker
    N_BOOL
}BOOL_FB;


/**
 * @brief what changed since the last cycle
 */
typedef enum{
    CHANGED_I = 0x1,
    CHANGED_O = 0x2,
    CHANGED_M = 0x4,
    CHANGED_T = 0x8,
    CHANGED_S = 0x10,
    CHANGED_STATUS = 0x20
}CHANGE_DELTA; 

/***********************plc_t*****************************/
/**
 * @brief The digital_input struct
 */
typedef struct digital_input{
    BYTE I;///contact value
    BYTE RE;///rising edge
    BYTE FE;///falling edge
    char nick[NICKLEN];///nickname
} * di_t;

/**
 * @brief The digital_output struct
 */
typedef struct digital_output{
    BYTE Q;//contact
    BYTE SET;//set
    BYTE RESET;//reset
    char nick[NICKLEN];//nickname
} * do_t;

/**
 * @brief The analog_io  struct
 */
typedef struct analog_io{
    double V;/// data value
    double min; ///range for conversion to/from raw data
    double max;
    char nick[NICKLEN];///nickname
} * aio_t;

/**
 * @brief The timer struct.
 * struct which represents  a timer state at a given cycle
 */
typedef struct timer{
    long S;	///scale; S=1000=>increase every 1000 cycles. STEP= 10 msec=> increase every 10 sec
    long sn;///internal counter used for scaling
    long V;	///value
    BYTE Q;	///output
    long P;	///Preset value
    BYTE ONDELAY;///1=on delay, 0 = off delay
    BYTE START;///start command: must be on to count
    //BYTE RESET;///down command: sets V = 0
    char nick[NICKLEN];
} * dt_t;

/**
 * @brief The blink struct
 * struct which represents a blinker
 */
typedef struct blink{
    BYTE Q; ///output
    long S;	///scale; S=1000=>toggle every 1000 cycles. STEP= 10 msec=> toggle every 10 sec
    long sn;///internal counter for scaling
    char nick[NICKLEN];
} * blink_t;

/**
 * @brief The mvar struct
 * struct which represents a memory register / counter
 */
typedef struct mvar{
    uint64_t V;     ///TODO: add type
    BYTE RO;	///1 if read only;
    BYTE DOWN;	///1: can be used as a down counter
    BYTE PULSE;		///pulse for up/downcounting
    BYTE EDGE;		///edge of pulse
    BYTE SET;		///set pulse
    BYTE RESET;		///reset pulse
    char nick[NICKLEN];   ///nickname
} * mvar_t;

/**
 * @brief The mreal struct
 * struct which represents a real number memory register
 */
typedef struct mreal{
    double V;     ///TODO: add type
    BYTE RO;	///1 if read only;
    char nick[NICKLEN];   ///nickname
} * mreal_t;

/**
 * @brief The PLC_regs struct
 * The struct which contains all the software PLC registers
 */
typedef struct PLC_regs{
    char hw[MAXSTR]; ///just a label for the hardware used
    BYTE *inputs;   ///digital input values buffer
    uint64_t *real_in; ///analog raw input values buffer
    BYTE *outputs;  ///digital output values buffer
    uint64_t *real_out; ///analog raw output values buffer
    BYTE *edgein;	///edges buffer
    BYTE *maskin;	///masks used to force values
	BYTE *maskout;
	BYTE *maskin_N;
	BYTE *maskout_N;
	double *mask_ai;
	double *mask_aq;
    BYTE command;   ///serial command from plcpipe
    BYTE response;  ///response to named pipe
    BYTE update; ///binary mask of state update
    int status;    ///0 = stopped, 1 = running, negative = error
	
	BYTE ni; ///number of bytes for digital inputs 
	di_t di; ///digital inputs
	
	BYTE nq; ///number of bytes for digital outputs
	do_t dq; ///the digital outputs

    BYTE nai; ///number of analog input channels
	aio_t ai; ///the analog inputs
	
	BYTE naq; ///number of analog output channels
	aio_t aq; ///the analog outputs
	
	BYTE nt; ///number of timers
	dt_t t; ///the timers
    
    BYTE ns; ///number of blinkers
	blink_t s; ///the blinkers
	
	BYTE nm; ///number of memory counters
	mvar_t m; ///the memory
	
	BYTE nmr; ///number of memory registers
	mreal_t mr; ///the memory

	rung_t * rungs;
	BYTE rungno; //256 rungs should suffice
	
	long step; //cycle time in milliseconds
	char * response_file; //pipe to send response.
	struct pollfd com[1];  //polling on a file descriptor for external 
	                        //"commands", this will be thrown away and  
	                        //replaced by usleep
	                        //FIXME: throw this feature away
	struct PLC_regs * old; //pointer to previous state
} * plc_t;


plc_t plc_start(plc_t p);

plc_t plc_stop(plc_t p);

/**
 * @brief execute JMP instruction
 * @param the rung
 * @param the program counter (index of instruction in the rung)
 * @return OK or error
 */
int handle_jmp( const rung_t r, unsigned int * pc);

/**
 * @brief execute RESET instruction
 * @param the instruction
 * @param current acc value
 * @param true if we are setting a bit from a variable, 
 * false if we are setting the input of a block
 * @param reference to the plc
 * @return OK or error
 */
int handle_reset( const instruction_t op,
                  const data_t acc,
                  BYTE is_bit,    
                  plc_t p);

/**
 * @brief execute SET instruction
 * @param the instruction
 * @param current acc value
 * @param true if we are setting a bit from a variable, 
 * false if we are setting the input of a block
 * @param reference to the plc
 * @return OK or error
 */
int handle_set( const instruction_t op,
                const data_t acc, 
                BYTE is_bit,
                plc_t p);

/**
 * @brief store value to digital outputs
 * @note values are stored in BIG ENDIAN
 * @param value
 * @reference to the plc
 * @return OK or error
 */
int st_out( const instruction_t op, 
            uint64_t val,
            plc_t p);
          
/**
 * @brief store value to analog outputs
 * @param value
 * @reference to the plc
 * @return OK or error
 */            
int st_out_r( const instruction_t op, 
              double val,
              plc_t p);

/**
 * @brief store value to memory registers
 * @param value
 * @reference to the plc
 * @return OK or error
 */
int st_mem( const instruction_t op, 
            uint64_t val,
            plc_t p);

/**
 * @brief store value to floating point memory registers 
 * @param value
 * @reference to the plc
 * @return OK or error
 */
int st_mem_r( const instruction_t op, 
              double val,
              plc_t p);


/**
 * @brief execute STORE instruction
 * @param the value to be stored
 * @param the instruction
 * @param reference to the plc
 * @return OK or error
 */
int handle_st( const instruction_t op, 
               const data_t val, 
               plc_t p);
                
/**
 * @brief load value from digital inputs
 * values are loaded in BIG ENDIAN
 * @param value
 * @reference to the plc
 * @return OK or error
 */
int ld_in( const instruction_t op, 
           uint64_t * val,
           plc_t p);
            
/**
 * @brief load rising edge from digital inputs
 * @param value
 * @reference to the plc
 * @return OK or error
 */
int ld_re( const instruction_t op, 
           BYTE * val,
           plc_t p);
            
/**
 * @brief load falling edge from digital inputs
 * @param value
 * @reference to the plc
 * @return OK or error
 */            
int ld_fe( const instruction_t op, 
           BYTE * val,
           plc_t p);
            
/**
 * @brief load value from analog inputs
 * @param value
 * @reference to the plc
 * @return OK or error
 */            
int ld_in_r( const instruction_t op, 
             double * val,
             plc_t p);
            
/**
 * @brief load value from digital outputs
 * @param value
 * @reference to the plc
 * @return OK or error
 */
int ld_out( const instruction_t op, 
            uint64_t * val,
            plc_t p);
          
/**
 * @brief load value from analog outputs
 * @param value
 * @reference to the plc
 * @return OK or error
 */            
int ld_out_r( const instruction_t op, 
              double * val,
              plc_t p);

/**
 * @brief load value from memory registers
 * @param value
 * @reference to the plc
 * @return OK or error
 */
int ld_mem( const instruction_t op, 
            uint64_t * val,
            plc_t p);

/**
 * @brief load value from floating point memory 
 * @param value
 * @reference to the plc
 * @return OK or error
 */
int ld_mem_r( const instruction_t op, 
              double * val,
              plc_t p);
            
/**
 * @brief load value from timer 
 * @param value
 * @reference to the plc
 * @return OK or error
 */
int ld_timer( const instruction_t op, 
              uint64_t * val,
              plc_t p);            

/**
 * @brief execute LOAD instruction
 * @param the instruction
 * @param where to load the value
 * @param reference to the plc
 * @return OK or error
 */
int handle_ld( const instruction_t op, 
               data_t * acc, 
               plc_t p);

/**
 * @brief execute any stack operation
 * @param the intruction
 * @param the rung
 * @param reference to the plc
 * @return OK or error
 */
int handle_stackable(const instruction_t op,
                     rung_t r, 
                     plc_t p);

/**
 * @brief execute IL instruction
 * @param the plc
 * @param the rung
 * @return OK or error
 */
int instruct(plc_t p, rung_t r, unsigned int *pc);

/**
 * @brief add a new rung to a plc
 * @param the plc
 * @return reference to the new rung
 */
rung_t mk_rung( plc_t p);

/**
 * @brief get rung reference from plc
 * @param p a plc
 * @param r the rung reference
 * @param idx the index
 * @return OK or error
 */
int get_rung( const plc_t p, unsigned int idx, rung_t *r);

/**
 * @brief task to execute IL rung
 * @param timeout (usec)
 * @param pointer to PLC registers
 * @param pointer to IL rung
 * @return OK or error
 */
int task( long timeout, plc_t p, rung_t r);

/**
 * @brief task to execute all rungs
 * @param timeout (usec)
 * @param pointer to PLC registers
 * @return OK or error
 */
int all_tasks( long timeout, plc_t p);

/**
 * @brief custom project init code as plugin
 * @return OK or error
 */
int project_init();

/**
 * @brief custom project task code as plugin
 * @param pointer to PLC registers
 * @return OK or error
 */
int project_task( plc_t p);

/**
 * @brief PLC task executed in a loop
 * @param pointer to PLC registers
 * @return OK or error code
 */
int plc_task( plc_t p);

/**
 * @brief Pipe initialization executed once
 * @param pipe to poll
 * @param ref to plc
 * @return OK or error
 */
int open_pipe(const char * pipe, plc_t p);

/**
 * @brief PLC realtime loop
 * Anything in this function normally (ie. when not in error)
 * satisfies the realtime @conditions:
 * 1. No disk I/O
 * 2. No mallocs
 * This way the time it takes to execute is predictable
 * Heavy parts can timeout
 * The timing is based on poll.h
 * which is also realtime when using a preempt scheduler
 * @param the PLC
 * @return PLC with updated state
 */
plc_t plc_func( plc_t p);

/**
 * @brief is input forced
 * @param reference to plc
 * @param input index
 * @return true if forced, false if not, error if out of bounds
 */
int is_input_forced(const plc_t p, BYTE i);

/**
 * @brief is output forced
 * @param reference to plc
 * @param input index
 * @return true if forced, false if not, error if out of bounds
 */
int is_output_forced(const plc_t p, BYTE i);


/**
 * @brief decode inputs
 * @param pointer to PLC registers
 * @return true if input changed
 */
unsigned char dec_inp( plc_t p);

/**
 * @brief encode outputs
 * @param pointer to PLC registers
 * @return true if output changed
 */
unsigned char enc_out( plc_t p);

/**
 * @brief write values to memory variables
 * @param pointer to PLC registers
 */
void write_mvars( plc_t p);

/**
 * @brief read_mvars
 * @param pointer to PLC registers
 */
void read_mvars( plc_t p);

/**
 * @brief rising edge of input
 * @param pointer to PLC registers
 * @param type is Digital Input Or Counter
 * @param index
 * @return true if rising edge of operand, false or error otherwise
 */
int re( const plc_t p, 
        int type, 
        int idx);

/**
 * @brief falling edge of input
 * @param pointer to PLC registers
 * @param type is Digital Input Or Counter
 * @param index
 * @return true if falling edge of operand, false or error otherwise
 */
int fe( const plc_t p, 
        int type, 
        int idx);

/**
 * @brief set output
 * @param pointer to PLC registers
 * @param type is Digital Output, Timer or Counter
 * @param index
 * @return OK if success or error code
 */
int set( plc_t p, 
         int type, 
         int idx);

/**
 * @brief reset output
 * @param pointer to PLC registers
 * @param type is Digital Output, Timer or Counter
 * @param index
 * @return OK if success or error code
 */
int reset( plc_t p, 
           int type,
           int idx);

/**
 * @brief contact value to output
 * @param pointer to PLC registers
 * @param type is Digital Output, Timer or Counter
 * @param index
 * @param value
 * @return OK if success or error code
 */
int contact( plc_t p, 
             int type, 
             int idx, 
             BYTE val);

/**
 * @brief resolve an operand value
 * @param pointer to PLC registers
 * @param type of operand
 * @param index
 * @return return value or error code
 */
int resolve( plc_t p, 
             int type, 
             int idx);

/**
 * @brief reset timer
 * @param pointer to PLC registers
 * @param index
 * @return OK if success or error code
 */
int down_timer( plc_t p, int idx);

/**
 * @brief construct a new plc with a configuration
 * @param number of digital inputs
 * @param number of digital outputs
 * @param number of analog inputs 
 * @param number of analog outputs
 * @param number of timers
 * @param number of pulses
 * @param number of integer memory variables
 * @param number of real memory variables
 * @param cycle time in milliseconds
 * @param hardware identifier        

 * @return configured plc
 */
plc_t new_plc(
    int di, 
    int dq,
    int ai,
    int aq,
    int nt, 
    int ns,
    int nm,
    int nr,
    int step,
    const char * hw);

/**
 * @brief copy constructor
 * @param source plc
 * @return newly allocated copy
 */
plc_t copy_plc(const plc_t plc); 

/*configurators*/
//plc_t declare_input(const plc_t p,  BYTE idx, const char* val);

/**
 * @brief assign name to a plc register variable
 * @param plc instance   
 * @param the type of variable (IL_OPERANDS enum value)
 * @param variable index
 * @param variable name
 * @see also data.h
 * @return plc instance with saved change or updated error status
 */
plc_t declare_variable(const plc_t p, 
                        int var,
                        BYTE idx,                          
                        const char* val);

/**
 * @brief assign initial value to a plc register variable
 * @param plc instance   
 * @param the type of variable (IL_OPERANDS enum value)
 * @param variable index
 * @param variable initial value (serialized float or long eg.
 * "5.35" or "12345")
 * @see also data.h
 * @return plc instance with saved change or updated error status
 */
plc_t init_variable(const plc_t p, int var, BYTE idx, const char* val);

/**
 * @brief configure a plc register variable as readonly
 * @param plc instance   
 * @param the type of variable (IL_OPERANDS enum value)
 * @param variable index
 * @param serialized readonly flag (true if "TRUE", false otherwise)
 * @see also data.h
 * @return plc instance with saved change or updated error status
 */
plc_t configure_variable_readonly(const plc_t p, 
                                int var, 
                                BYTE idx, 
                                const char* val);
                                
/**
 * @brief assign upper or lower limit to an analog input or output
 * @param plc instance   
 * @param the type of io (IL_OPERANDS enum value)
 * @param variable index
 * @param the limit value (serialized float eg. "5.35")
 * @param upper limit if true, otherwise lower limit
 * @see also data.h
 * @return plc instance with saved change or updated error status
 */
plc_t configure_io_limit(const plc_t p, 
                        int io, 
                        BYTE idx, 
                        const char* val,
                        BYTE max);                                

/**
 * @brief configure a register as up or down counter
 * @param plc instance   
 * @param variable index
 * @param serialized direction flag (true if "DOWN", false otherwise)
 * @return plc instance with saved change or updated error status
 */
plc_t configure_counter_direction(const plc_t p, 
                                    BYTE idx, 
                                    const char* val);
/**
 * @brief configure a timer scale
 * @param plc instance   
 * @param timer index
 * @param serialized long (eg 100000)
 * @see also timer_t
 * @return plc instance with saved change or updated error status
 */
plc_t configure_timer_scale(const plc_t p, 
                     BYTE idx, 
                     const char* val);

/**
 * @brief configure a timer preset
 * @param plc instance   
 * @param timer index
 * @param serialized long (eg 100000)
 * @see also timer_t
 * @return plc instance with saved change or updated error status
 */
plc_t configure_timer_preset(const plc_t p, 
                        BYTE idx, 
                        const char* val);

/**
 * @brief configure a timer delay mode
 * @param plc instance   
 * @param timer index
 * @param serialized mode flag (true if "ON", false otherwise)
 * @see also timer_t
 * @return plc instance with saved change or updated error status
 */
plc_t configure_timer_delay_mode(const plc_t p, 
                        BYTE idx, 
                        const char* val);

/**
 * @brief configure a pulse scale
 * @param plc instance   
 * @param pulse index
 * @param serialized long (eg 100000)
 * @see also blink_t
 * @return plc instance with saved change or updated error status
 */
plc_t configure_pulse_scale(const plc_t p, 
                        BYTE idx, 
                        const char* val);

#endif //_PLCLIB_H_
