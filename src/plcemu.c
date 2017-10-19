#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>

#include "plcemu.h"
#include "config.h"
#include "util.h"
#include "data.h"
#include "instruction.h"
#include "rung.h"
#include "plclib.h"
#include "parser-tree.h"
#include "parser-il.h"
#include "parser-ld.h"
#include "ui.h"
#include "hardware.h"



/*************GLOBALS************************************************/
plc_t Plc;
char Lines[MAXBUF][MAXSTR];///program lines
int Lineno;    ///actual no of active lines
int UiReady=FALSE;
BYTE Update=FALSE;

int Language = LANG_LD;

typedef enum {
    MSG_PLCERR,
    MSG_BADCHAR,
    MSG_BADFILE,
    MSG_BADOPERAND,
    MSG_BADINDEX,
    MSG_BADCOIL,
    MSG_BADOPERATOR,
    MSG_TIMEOUT,
    MSG_OVFLOW,
    N_ERRMSG
}ERRORMESSAGES;

const char ErrMsg[N_ERRMSG][MEDSTR] = {
        "Something went terribly wrong!",
        "Invalid Symbol!",
        "Wrong File!",
        "Invalid Operand!",
        "Invalid Numeric!",
        "Invalid Output!",
        "Invalid Command!",
        "Timeout!",
        "Stack overflow!"
};

const char LangStr[3][TINYSTR] ={
     "Ladder Diagram",
    "Instruction List",
    "Structured Text"
};


void sigenable() {
    ui_toggle_enabled();
}

/**
* @brief graceful shutdown
*/
void sigkill() {
    plc_log("program interrupted");
    double mean, var = 0;
    unsigned long loop = get_loop();
    get_variance(&mean, &var);
    plc_log("Total loops: %d", loop);
    plc_log("Average loop time: %f us", mean);
    plc_log("Standard deviation: +-%f us", sqrt(var));
    ui_end();
    exit(0);
}

plc_t declare_names(int operand,
                    const variable_t var,                         
                    plc_t plc){
    char * name = NULL;

    if((name = var->name)){
        return declare_variable(plc, operand, var->index, name); 
    } else return plc;
} 

plc_t configure_limits(int operand,                        
                       const variable_t var, 
                       plc_t plc){
    plc_t p = plc;
    char * max = NULL;
    char * min = NULL;
    
    if((max = get_param_val("MAX", var->params))){
        p = configure_io_limit(p, operand, var->index, max, TRUE);
    }
    if((min = get_param_val("MIN", var->params))){
        p = configure_io_limit(p, operand, var->index, min, FALSE);
    }   
    return p;                         
}

plc_t init_values(int operand,                        
                  const variable_t var, 
                  plc_t plc){
    char * val = NULL;
    
    if((val = get_param_val("VALUE", var->params))){
        return init_variable(plc, operand, var->index, val);
    }
    return plc;                         
}

plc_t configure_readonly(int operand,                        
                        const variable_t var, 
                        plc_t plc){
    char * val = NULL;
    
    if((val = get_param_val("READONLY", var->params))){
      return configure_variable_readonly(plc, operand, var->index, val);
    } 
    return plc;               
}


plc_t configure_di(const config_t conf, plc_t plc){
 
    sequence_t seq = get_sequence_entry(CONFIG_DI, conf);
   
    if(seq) {
        plc_t p = plc;
        int i = 0;
        for(; i < seq->size; i++){
        //names
            p = declare_names(OP_INPUT, &(seq->vars[i]), p);    
        }
        return p;
    } 
    return plc;
}

plc_t configure_dq(const config_t conf, plc_t plc){

    sequence_t seq = get_sequence_entry(CONFIG_DQ, conf); 
    
    if(seq) {
        plc_t p = plc;
        int i = 0;
        for(; i < seq->size; i++){
        //names
            p = declare_names(OP_OUTPUT, &(seq->vars[i]), p);    
        }
        return p;
    } 
    return plc;
}

plc_t configure_ai(const config_t conf, plc_t plc){

    sequence_t seq = get_sequence_entry(CONFIG_AI, conf);
    
    if(seq) {
        
        plc_t p = plc;
        int i = 0;
        for(; i < seq->size; i++){
            //names
            p = declare_names(OP_REAL_INPUT, &(seq->vars[i]), p);
            //limits    
            p = configure_limits(OP_REAL_INPUT, &(seq->vars[i]), p);
        }
        return p;
    } 
    return plc;
}

plc_t configure_aq(const config_t conf, plc_t plc){

    sequence_t seq = get_sequence_entry(CONFIG_AQ, conf);
    
    if(seq) {
        plc_t p = plc;
        int i = 0;
        for(; i < seq->size; i++){
            //names
            p = declare_names(OP_REAL_OUTPUT, &(seq->vars[i]), p);
            //limits    
            p = configure_limits(OP_REAL_OUTPUT, &(seq->vars[i]), p);
        }
        return p;
    } 
    return plc;
}

plc_t configure_counters(const config_t conf, plc_t plc){

    sequence_t seq = get_sequence_entry(CONFIG_MVAR, conf);
    
    if(seq) {
        plc_t p = plc;
        int i = 0;
        char * val = NULL;
        for(; i < seq->size; i++){
            //names
            p = declare_names(OP_MEMORY, &(seq->vars[i]), p);
            //defaults
            p = init_values(OP_MEMORY, &(seq->vars[i]), p);
            //readonlies
            p = configure_readonly(OP_MEMORY, &(seq->vars[i]), p);
            //directions    
            if((val = get_param_val("COUNT", seq->vars[i].params))){
                p = configure_counter_direction(p, i, val);
                val = NULL;
            }
        }
        return p;   
    } 
    return plc;
}

plc_t configure_reals(const config_t conf, plc_t plc){

    sequence_t seq = get_sequence_entry(CONFIG_MREG, conf);
    
    if(seq) {
        plc_t p = plc;
        int i = 0;
        for(; i < seq->size; i++){
            //names
            p = declare_names(OP_REAL_MEMORY, &(seq->vars[i]), plc);
            //defaults
            p = init_values(OP_REAL_MEMORY, &(seq->vars[i]), p);
            //readonlies
            p = configure_readonly(OP_REAL_MEMORY, &(seq->vars[i]), p); 
        }
        return p; 
    } 
    return plc;
}

plc_t configure_timers(const config_t conf, plc_t plc){

    sequence_t seq = get_sequence_entry(CONFIG_TIMER, conf);
    
    if(seq) {
        int i = 0;
        char * scale = NULL;
        char * preset = NULL;
        char * ondelay = NULL;
        plc_t p = plc;
        for(; i < seq->size; i++){
            //names
            p = declare_names(OP_TIMEOUT, &(seq->vars[i]), plc);
            //scales
            if((scale = get_param_val("RESOLUTION", 
                                        seq->vars[i].params))){
                p = configure_timer_scale(p, i, scale);
                scale = NULL;
            }
            //presets            
            if((preset = get_param_val("PRESET", 
                                        seq->vars[i].params))){
                p = configure_timer_preset(p, i, preset);
                preset = NULL;
            }
            //modes
            if((ondelay = get_param_val("ONDELAY", 
                                        seq->vars[i].params))){
                p = configure_timer_delay_mode(p, i, ondelay);
                ondelay = NULL;
            }
        }   
        return p; 
    } 
    return plc;    
}

plc_t configure_pulses(const config_t conf, plc_t plc){

    sequence_t seq = get_sequence_entry(CONFIG_PULSE, conf);
    
    if(seq) {
        int i = 0;
        char * scale = NULL;
        plc_t p = plc;
        for(; i < seq->size; i++){
            //names
            p = declare_names(OP_BLINKOUT, &(seq->vars[i]), plc);
            //scales
            if((scale = get_param_val("RESOLUTION",
                                     seq->vars[i].params))){
                p = configure_pulse_scale(p, i, scale);
                scale = NULL;
            }
        }   
        return p; 
    } 
    return plc;    
}


plc_t configure(const config_t conf, plc_t plc){

    plc_t p = plc;
    
    p = configure_di(conf, p);
    p = configure_dq(conf, p);
    p = configure_ai(conf, p);
    p = configure_aq(conf, p);
    p = configure_counters(conf, p);
    p = configure_reals(conf, p);
    p = configure_timers(conf, p);
    p = configure_pulses(conf, p);
    
    return p;
}

plc_t init_emu(const config_t conf) {
   
    hw_config(conf);
        
    int di = get_sequence_entry(CONFIG_DI, conf)->size; 
    int dq = get_sequence_entry(CONFIG_DQ, conf)->size;
    int ai = get_sequence_entry(CONFIG_AI, conf)->size;
    int aq = get_sequence_entry(CONFIG_AQ, conf)->size;
    int nt = get_sequence_entry(CONFIG_TIMER, conf)->size;
    int ns = get_sequence_entry(CONFIG_PULSE, conf)->size;
    int nm = get_sequence_entry(CONFIG_MREG, conf)->size;
    int nr = get_sequence_entry(CONFIG_MVAR, conf)->size;
    int step = get_numeric_entry(CONFIG_STEP, conf);
    char * hw = get_string_entry(CONFIG_HW, conf);
    
    plc_t p = new_plc(di, dq, ai, aq, nt, ns, nm, nr, step, hw);
    p = configure(conf, p);
    
    Update = TRUE;
    
    //signal(conf->sigenable, sigenable);
    signal(SIGINT, sigkill);
    signal(SIGTERM, sigkill);
    
    project_init();
    
    open_pipe(get_string_entry(CONFIG_HW, conf), p);
    return p;
}

int load_program_file(const char * path, plc_t plc) {
    FILE * f;
    int r = ERR_BADFILE;
    char line[MAXSTR];
    int i=0;
    
    if(path == NULL)
        return r;
    char * ext = strrchr(path, '.');
    int lang = PLC_ERR;
    
    if(ext != NULL){
        if(strcmp(ext, ".il") == 0){
            lang = LANG_IL;
        }else if(strcmp(ext, ".ld") == 0) {
            lang = LANG_LD;
        }
    }          
    if (lang > PLC_ERR && (f = fopen(path, "r"))) {
        memset(line, 0, MAXSTR);
       
        while (fgets(line, MAXSTR, f)) {
            memset(Lines[i], 0, MAXSTR);
            sprintf(Lines[i++], "%s", line);
        }
        Lineno = i;
        r = PLC_OK;
    } 
    if(r > PLC_ERR){
        if(lang == LANG_IL){
            r = parse_il_program(Lines, plc);
        }else{ 
            r = parse_ld_program(Lines, plc);   
        }
    }
    return r;
}

const char * Usage = "Usage: plcemu [-c config file] \n \
        Options:\n \
        -h displays this help message\n \
        -c uses a configuration file other than config.yml";

void print_error(int errcode)
{
    const char * errmsg;
    switch(errcode) {
        case ERR_BADCHAR:
            errmsg = ErrMsg[MSG_BADCHAR];
            break;
        case ERR_BADFILE:
            errmsg = ErrMsg[MSG_BADFILE];
            break;
        case ERR_BADOPERAND:
            errmsg = ErrMsg[MSG_BADOPERAND];
            break;
        case ERR_BADINDEX:
            errmsg = ErrMsg[MSG_BADINDEX];
            break; 
        case ERR_BADCOIL:
            errmsg = ErrMsg[MSG_BADCOIL];
            break;
        case ERR_BADOPERATOR:
            errmsg = ErrMsg[MSG_BADOPERATOR];
            break;
        case ERR_TIMEOUT:
            errmsg = ErrMsg[MSG_TIMEOUT];
            break;   
        case ERR_OVFLOW:
            errmsg = ErrMsg[MSG_OVFLOW];
            break;                 
        default://PLC_ERR
            errmsg = ErrMsg[MSG_PLCERR];
    }
    plc_log("error %d: %s", -errcode, errmsg);
}

config_t init_config(){
 //TODO: in a c++ implementation this all can be done automatically 
 //using a hashmap
    config_t conf = new_config(N_CONFIG_VARIABLES);
   
    config_t uspace = new_config(N_USPACE_VARS);
        
    uspace = update_entry(
        USPACE_BASE,
	    new_entry_int(50176, "BASE"),
	    uspace);
	
	uspace = update_entry(
	    USPACE_WR, 
	    new_entry_int(0, "WR"),
	    uspace);
	    
	uspace = update_entry(
	    USPACE_RD, 
	    new_entry_int(8, "RD"),
	    uspace);
	
	config_t subdev = new_config(N_SUBDEV_VARS);
	
    subdev = update_entry(
        SUBDEV_IN,
	    new_entry_int(0, "IN"),
	    subdev);
	    
	subdev = update_entry(
	    SUBDEV_OUT,
	    new_entry_int(1, "OUT"),
	    subdev);
	    
    subdev = update_entry(
        SUBDEV_ADC, 
	    new_entry_int(2, "ADC"),
	    subdev);
	    
	subdev = update_entry(
	    SUBDEV_DAC, 
	    new_entry_int(3, "DAC"),
	    subdev);
	
	config_t comedi = new_config(N_COMEDI_VARS);
	
	comedi = update_entry(
	    COMEDI_FILE,
	    new_entry_int(0, "FILE"),
	    comedi);
	    
	comedi = update_entry(
	    COMEDI_SUBDEV, 
	    new_entry_map(subdev, "SUBDEV"),
	    comedi);
    
    config_t sim = new_config(N_SIM_VARS);
    
    sim = update_entry(
        SIM_INPUT,
        new_entry_str("sim.in", "INPUT"), 
        sim);
        
    sim = update_entry(
        SIM_OUTPUT,
        new_entry_str("sim.out", "OUTPUT"),
        sim);    

    conf = update_entry(
        CONFIG_STEP,
        new_entry_int(1, "STEP"),
        conf);
    
    conf = update_entry(
        CONFIG_PIPE,
        new_entry_str("plcpipe", "PIPE"),
        conf);
    
    conf = update_entry(
        CONFIG_HW,
        new_entry_str("STDI/O", "HW"),
        conf);
        
    conf = update_entry(
        CONFIG_USPACE,
        new_entry_map(uspace, "USPACE"),
        conf);
    
    conf = update_entry(
        CONFIG_COMEDI,
        new_entry_map(comedi, "COMEDI"),
        conf);
    
    conf = update_entry(
        CONFIG_SIM,
        new_entry_map(sim, "SIM"),
        conf);
        
     conf = update_entry(
        CONFIG_PROGRAM_IL,
        new_entry_str("", "IL"),
        conf);   
    
    conf = update_entry(
        CONFIG_PROGRAM_LD,
        new_entry_str("", "LD"),
        conf);         
   /*******************************************/
   
    conf = update_entry(
        CONFIG_TIMER,
        new_entry_seq(new_sequence(4), "TIMERS"),
        conf);
    
    conf = update_entry(
        CONFIG_PULSE,
        new_entry_seq(new_sequence(4), "PULSES"),
        conf);
        
    conf = update_entry(
        CONFIG_MREG,
        new_entry_seq(new_sequence(4), "MREG"),
        conf);
        
    conf = update_entry(
        CONFIG_MVAR,
        new_entry_seq(new_sequence(4), "MVAR"),
        conf);
    
    conf = update_entry(
        CONFIG_DI,
        new_entry_seq(new_sequence(8), "DI"),
        conf);
 
    conf = update_entry(
        CONFIG_DQ,
        new_entry_seq(new_sequence(8), "DQ"),
        conf);
    
    conf = update_entry(
        CONFIG_AI,
        new_entry_seq(new_sequence(8), "AI"),
        conf);
    
    conf = update_entry(
        CONFIG_AQ,
        new_entry_seq(new_sequence(8), "AQ"),
        conf);

    return conf;
}

config_t init_command(config_t conf){
    config_t com = new_config(N_CLI_ARGS);
    int i = CLI_AI;
    for(; i < N_CLI_ARGS; i++){
        com = update_entry(i,
            copy_entry(get_entry(i, conf)),
            com);
    }
}

int main(int argc, char **argv)
{
    int errcode = PLC_OK;
    int more = 0;
    char * confstr = "config.yml";
    config_t conf = init_config();
    config_t command = init_command(conf);
    config_t state = init_command(conf);
    char * cvalue = NULL;
    opterr = 0;
    int c;
    while ((c = getopt (argc, argv, "hc:")) != PLC_ERR){
        switch (c) {
            case 'h':
                 plc_log(Usage);
                 return 1;
                break;
            case 'c':
                cvalue = optarg;
                break;
            case '?':
                plc_log(Usage);
                if (optopt == 'c'){
                    plc_log( 
                    "Option -%c requires an argument\n", optopt);
                } else if (isprint (optopt)){
                    plc_log(
                    "Unknown option `-%c'\n", optopt);
                } else{
                    plc_log(
                    "Unknown option character `\\x%x'\n",
                    optopt);
                }
                return 1;
            default:
               
                abort ();
        }
    }
    if(cvalue == NULL)
       cvalue = confstr;
       
    if ((load_config_yml(cvalue, conf))->err < PLC_OK) {
        plc_log("Invalid configuration file %s\n", cvalue);
        return PLC_ERR;
    }
//initialize PLC
    Plc = init_emu(conf);
//start hardware
    enable_bus();
//start UI
    more = ui_init();
    UiReady=more;
    
    load_program_file(get_string_entry(CONFIG_PROGRAM_IL,conf), Plc);   
    load_program_file(get_string_entry(CONFIG_PROGRAM_LD,conf), Plc);
    
    while (more > 0 ) {
       if(Update == TRUE){
       //state = get_state(Plc);
           ui_draw(state);
           
        }   
        command = ui_update();
        //apply(command, Plc)
        more = 1;
        if(errcode >= PLC_OK && Plc->status > 0){  
            errcode = plc_func(&Update, Plc);
            if(errcode < 0){
                print_error(errcode);
                Plc->status = 0;
            }
        }
    }
    
    disable_bus();
    clear_config(conf);
    close_log();
    ui_end();
    return 0;
}

