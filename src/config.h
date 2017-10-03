#ifndef _CONFIG_H_
#define _CONFIG_H_
#include <yaml.h>

#define ADVANTECH_HISTORICAL_BASE 50176
#define CONF_OK 0
#define CONF_ERR -1
#define CONF_STR 1024 //string length
#define CONF_MAX_SEQ 256 //max sequence elements
#define CONF_NUM 24 //number digits 
#define CONF_F 0
#define CONF_T 1

typedef enum {
    STORE_KEY,
    STORE_VAL,
    N_STORAGE_MODES
} STORAGE_MODE;

typedef enum {
    ENTRY_NONE,
	ENTRY_INT,
	ENTRY_STR,
	ENTRY_MAP,
	ENTRY_SEQ,
	N_ENTRY_TYPES
} ENTRY_TYPE;

typedef struct param {
    char * key; //min, max, default, preset, readonly, countdown, etc.
    char * value;
    struct param * next;
} * param_t;

#define MAX_PARAM 256
typedef param_t * param_table_t;

typedef struct variable {
    unsigned char index;
    char * name ;
/*    char * value;
  */
  param_t params;    
} * variable_t;

typedef struct sequence {
    int size;
    variable_t vars;
} * sequence_t;

typedef struct entry {
	int type_tag;
	char * name;
	
	union {
	    int scalar_int;
		char * scalar_str;
		sequence_t seq;
		struct config * conf;
	} e;
} * entry_t;

typedef entry_t * entry_map_t;

typedef struct config {
    unsigned int size;
    int err;
    entry_map_t map;
} * config_t;

/**
 * @brief construct a sequence
 * @param the size of the sequence
 * @return a newly alloced sequence
 */
sequence_t new_sequence(int size);

/**
 * @brief construct  configuration
 * @param the size of the entry map
 * @return a newly alloced config
 */
config_t new_config(int size);

/**
 * @brief cleanup and free configuration
 * @param the configuration
 * @return NULL
 */
config_t clear_config(config_t c);

/**
 * @brief entry point: load text file into configuration
 * @param filename (full path)
 * @param the configuration
 * @return new config
 */
config_t load_config_yml(const char * filename, config_t conf);

/**
 * @brief entry point: save configuration to text file 
 * @param filename (full path)
 * @param the configuration
 * @return OK or ERR
 */
int save_config_yml(const char * filename, const config_t conf);

/**
 * @brief new numeric entry
 * @param numeric value
 * @param entry map key
 * @return newly allocated entry
 */
entry_t new_entry_int(int i, char * name);

/**
 * @brief new string entry
 * @param str value
 * @param entry map key
 * @return newly allocated entry
 */
entry_t new_entry_str(char * str, char * name);

/**
 * @brief new recursive map entry
 * @param the map
 * @param entry map key
 * @return newly allocated entry
 */
entry_t new_entry_map(config_t map, char * name);

/**
 * @brief new fixed length sequence entry
 * @param the sequence
 * @param entry map key
 * @return newly allocated entry
 */
entry_t new_entry_seq(sequence_t seq, char * name);

/**
 * @brief new empty entry (for debugging only)
 * @return newly allocated entry
 */
entry_t new_entry_null();

/**
 * @brief update and replace an entry in a configuration
 * @param the entry key
 * @param the new entry
 * @param the configuration
 * @return the updated configuration
 */
config_t update_entry(unsigned int key, 
                      const entry_t item,
                      const config_t conf);

/**
 * @brief get config entry by key
 */
entry_t get_entry(int key, const config_t conf);

/**
 * @brief get numeric config entry by key
 */
int get_numeric_entry(int key, const config_t conf);

/**
 * @brief get string config entry by key
 */
char * get_string_entry(int key, const config_t conf);

/**
 * @brief get sequence config entry by key
 */
sequence_t get_sequence_entry(int key, const config_t conf);

/**
 * @brief get recursive map config entry by key
 */
config_t get_recursive_entry(int key, const config_t conf);

/**
 * @brief get config key by literal value
 */
int get_key(const char * value, const config_t conf);

/**
 * @brief get param key by literal value
 */
char * get_param_val(const char * key, const param_t params);

/**
 * @brief get param by literal value
 */
param_t get_param(const char * key, const param_t params);

/**
 * @brief append param
 */
param_t append_param(const param_t params, 
                     const char * key, 
                     const char * val);
/**
 * @brief append or update param value
 */
param_t update_param(const param_t params, 
                     const char * key, 
                     const char * val);

/**
 * @brief store a value to a map
 * @param the key
 * @param the value
 * @param where to store
 * @return config with applied value or changed errorcode 
 */
config_t store_value(
    unsigned char key, 
    const char * value, 
    config_t c);

/**
 * @brief store a value to a map that is nested in a sequence
 * @param the sequence 
 * @param the sequence index
 * @param the key
 * @param the value
 * @param where to store
 * @return config with applied value or changed errorcode
 */
config_t store_seq_value(unsigned char seq,
                    unsigned char idx, 
                    const char * key, 
                    const char * value, 
                    config_t c);

/**
 * @brief process an initialized parser, recursively for each mapping
 * @param sequence no. if the mapping is inside of a sequence, 
 * PLC_ERR otherwise
 * @param the parser
 * @param the configuration where the parsed values are stored
 * @return config with applied values or changed errorcode
 */
config_t process(int sequence, 
            yaml_parser_t *parser, 
            config_t conf);

/**
 * @brief emit all configuration values to a yaml emitter
 * @param the emitter
 * @param the configuration where the parsed values are stored
 * @return OK or ERR
 */
 
int emit(yaml_emitter_t *emitter,
         const config_t conf);
 
#endif //_CONFIG_H_




































