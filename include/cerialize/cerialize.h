#ifndef json_h
#define json_h

#include <stdlib.h>
#include <stdio.h>
#include <string.h>


typedef unsigned int cereal_size_t;
typedef unsigned int cereal_uint_t;

#define TRUE 1
#define FALSE 0

#define JSON_MAX_STRING_LENGTH 1024

typedef char bool_t;

typedef enum json_type {
    JSON_OBJECT,
    JSON_STRING,
    JSON_NUMBER,
    JSON_BOOL,
    JSON_NULL,
    JSON_LIST
} json_type;

typedef struct json_list {
    cereal_size_t count;
    struct json_object* items; // array of json_object
} json_list;

typedef struct json_body {
    struct json_node* nodes;
    cereal_size_t node_count;
} json_body;

typedef union json_value {
        char* string;
        float number;
        bool_t boolean;
        bool_t is_null;
        json_list list; // pointer to json_list
        json_body object;
} json_value; 

// need to define json_object that tracks the type, and then moodify the lexer to return this
typedef struct json_object {
    json_type type;
    json_value value;
} json_object;

typedef struct json_node {
    char* key;
    json_object value;
} json_node;

typedef struct {
    json_object root;
    char* error_text;
    cereal_size_t error_length;
    bool_t failure;
} json;

static inline char* serialize_json(const json* j);
static inline json deserialize_json(const char* json_string, cereal_size_t length);

// Memory management functions
static inline void json_free(json* j);
static inline void json_object_free(json_object* obj);

#define JSON_MAX_ERROR_LENGTH 512

// lexer
enum lex_type {
    LEX_QUOTE = '\"',
    LEX_COMMA = ',',
    LEX_PERIOD = '.',
    LEX_OPEN_BRACE = '{',
    LEX_CLOSE_BRACE = '}',
    LEX_OPEN_SQUARE = '[',
    LEX_CLOSE_SQUARE = ']',
    LEX_COLON = ':',
    LEX_F = 'f',
    LEX_T = 't',
    LEX_N = 'n',
};

static inline bool_t is_whitespace(char cur) {
    return (cur == ' ' || cur == '\t' || cur == '\n' || cur == '\r');
}

static inline bool_t is_number(char cur) {
    return (cur >= '0' && cur <= '9');
}

// Accepts a sign or digit as the first character of a number
static inline bool_t is_number_start(char cur) {
    return (cur == '-' || cur == '+' || (cur >= '0' && cur <= '9'));
}

// skip whitespace until next node
static inline void skip_whitespace(const char* json_string, cereal_size_t length, cereal_uint_t* i) {
    while (is_whitespace(json_string[*i]) && *i < length) {
        (*i)++;
    }
}

// string     : array of lex tokens representing string
// length     : length of string given
// i          : current parser index
// failure    : track whether parsing failed
// error_text : error text to append to if the parsing fails
// TODO: add parameter to store key length, maybe return a struct that gives this?  not sure.
static inline char* json_parse_string(const char* json_string, cereal_size_t length, cereal_uint_t* i, bool_t* failure, char* error_text) {

    // opening "'"
    if (json_string[*i] != LEX_QUOTE) {
        strcat(error_text, "cerialize ERROR: Expected quote to open JSON string.\n");
        *failure = TRUE;
        return NULL;
    }
    (*i)++;

    // find length of string so we allocate the proper size
    cereal_size_t str_size = 0;
    bool_t found_end = FALSE;
    bool_t found_newline = FALSE;
    cereal_uint_t j = *i;
    do {
        if (j >= length) {
            strcat(error_text, "cerialize ERROR: Expecting closing quote to close JSON string.\n");
            *failure = TRUE;
            return NULL;
        }
        if (json_string[j] == '\n' || json_string[j] == '\r') {
            found_newline = TRUE;
            break;
        }
        if (json_string[j] == LEX_QUOTE) {
            found_end = TRUE;
        } else {
            str_size++;
        }
    } while(!found_end && j++ < length);

    // Reject empty string
    if (str_size == 0) {
        strcat(error_text, "cerialize ERROR: Empty string not allowed.\n");
        *failure = TRUE;
        return NULL;
    }
    // Reject string with newline inside
    if (found_newline) {
        strcat(error_text, "cerialize ERROR: Newline in string not allowed.\n");
        *failure = TRUE;
        return NULL;
    }

    char* str = (char*)malloc(str_size + 1);  // +1 for null terminator
    for (cereal_uint_t j = 0; j < str_size; j++) {
        str[j] = json_string[*i];
        (*i)++;
    }
    str[str_size] = '\0';  // Add null terminator

    // closing "'"
    if (json_string[*i] != LEX_QUOTE) {
        strcat(error_text, "cerialize ERROR: Expected closing quote to close JSON string.\n");
        *failure = TRUE;
        return NULL;
    }
    (*i)++;

    return str;
}

static inline bool_t json_parse_null(const char* json_string, cereal_uint_t* i, bool_t* failure, char* error_text) {
    // check for "null"
    if (strncmp(&json_string[*i], "null", 4) != 0) {
        strcat(error_text, "cerialize ERROR: Expected 'null' keyword.\n");
        *failure = TRUE;
        return FALSE;
    }
    // Check next char is delimiter or end
    char next = json_string[*i + 4];
    if (next == '\0' || next == ',' || next == '}' || next == ']' || is_whitespace(next)) {
        *i += 4;
        // Check for extra null after delimiter (e.g. "null null")
        cereal_uint_t temp_i = *i;
        skip_whitespace(json_string, strlen(json_string), &temp_i);
        if (strncmp(&json_string[temp_i], "null", 4) == 0) {
            strcat(error_text, "cerialize ERROR: Multiple null values not allowed.\n");
            *failure = TRUE;
            return FALSE;
        }
        return TRUE;
    } else {
        strcat(error_text, "cerialize ERROR: Unexpected characters after 'null'.\n");
        *failure = TRUE;
        return FALSE;
    }
}

static inline float json_parse_number(const char* json_string, cereal_size_t length, cereal_uint_t* i, bool_t* failure, char* error_text) {
    // check for valid number format
    cereal_uint_t start = *i;
    // Accept optional sign at the start
    if (*i < length && (json_string[*i] == '-' || json_string[*i] == '+')) {
        (*i)++;
        // If another sign immediately follows, it's invalid
        if (*i < length && (json_string[*i] == '-' || json_string[*i] == '+')) {
            strcat(error_text, "cerialize ERROR: Multiple consecutive signs in number.\n");
            *failure = TRUE;
            return 0.0f;
        }
    }
    bool_t found_digit = 0;
    int period_count = 0;
    int e_count = 0;
    bool_t after_e = FALSE;
    while (*i < length && (is_number(json_string[*i]) || json_string[*i] == LEX_PERIOD || json_string[*i] == 'e' || json_string[*i] == 'E' || json_string[*i] == '-' || json_string[*i] == '+')) {
        if (is_number(json_string[*i])) {
            found_digit = 1;
            after_e = FALSE;
        }
        if (json_string[*i] == LEX_PERIOD) {
            period_count++;
            if (period_count > 1) {
                strcat(error_text, "cerialize ERROR: Multiple decimal points in number.\n");
                *failure = TRUE;
                return 0.0f;
            }
        }
        if (json_string[*i] == 'e' || json_string[*i] == 'E') {
            e_count++;
            after_e = TRUE;
            // Check for multiple e's
            if (e_count > 1) {
                strcat(error_text, "cerialize ERROR: Multiple exponents in number.\n");
                *failure = TRUE;
                return 0.0f;
            }
        }
        // If after 'e', next must be digit or sign
        if (after_e && (json_string[*i] != 'e' && json_string[*i] != 'E')) {
            if (!(is_number(json_string[*i]) || json_string[*i] == '-' || json_string[*i] == '+')) {
                strcat(error_text, "cerialize ERROR: Invalid exponent format in number.\n");
                *failure = TRUE;
                return 0.0f;
            }
            after_e = FALSE;
        }
        (*i)++;
    }

    if (!found_digit) {
        strcat(error_text, "cerialize ERROR: Expected number.\n");
        *failure = TRUE;
        return 0.0f;
    }
    // If last char was 'e' or 'E', fail (incomplete exponent)
    if (*i > start && (json_string[*i-1] == 'e' || json_string[*i-1] == 'E')) {
        strcat(error_text, "cerialize ERROR: Incomplete exponent in number.\n");
        *failure = TRUE;
        return 0.0f;
    }
    // extract number substring 
    float value = strtof(&json_string[start], NULL);
    return value;
}

static inline bool_t json_parse_boolean(const char* json_string, cereal_uint_t* i, bool_t* failure, char* error_text) {
    // Explicitly reject '1' and '0' as booleans
    if (json_string[*i] == '1' && (json_string[*i+1] == '\0' || is_whitespace(json_string[*i+1]) || json_string[*i+1] == ',' || json_string[*i+1] == '}' || json_string[*i+1] == ']')) {
        strcat(error_text, "cerialize ERROR: '1' is not a valid boolean value.\n");
        *failure = TRUE;
        return FALSE;
    }
    if (json_string[*i] == '0' && (json_string[*i+1] == '\0' || is_whitespace(json_string[*i+1]) || json_string[*i+1] == ',' || json_string[*i+1] == '}' || json_string[*i+1] == ']')) {
        strcat(error_text, "cerialize ERROR: '0' is not a valid boolean value.\n");
        *failure = TRUE;
        return FALSE;
    }

    // Accept only 'true' or 'false' (case-sensitive) followed by delimiter or end
    if (strncmp(&json_string[*i], "true", 4) == 0) {
        // Check next char is delimiter or end
        char next = json_string[*i + 4];
        if (next == '\0' || next == ',' || next == '}' || next == ']' || is_whitespace(next)) {
            *i += 4;
            // Check for extra boolean after delimiter (e.g. "true false")
            cereal_uint_t temp_i = *i;
            skip_whitespace(json_string, strlen(json_string), &temp_i);
            if (strncmp(&json_string[temp_i], "true", 4) == 0 || strncmp(&json_string[temp_i], "false", 5) == 0) {
                strcat(error_text, "cerialize ERROR: Multiple boolean values not allowed.\n");
                *failure = TRUE;
                return FALSE;
            }
            return TRUE;
        } else {
            strcat(error_text, "cerialize ERROR: Unexpected characters after 'true'.\n");
            *failure = TRUE;
            return FALSE;
        }
    } else if (strncmp(&json_string[*i], "false", 5) == 0) {
        char next = json_string[*i + 5];
        if (next == '\0' || next == ',' || next == '}' || next == ']' || is_whitespace(next)) {
            *i += 5;
            cereal_uint_t temp_i = *i;
            skip_whitespace(json_string, strlen(json_string), &temp_i);
            if (strncmp(&json_string[temp_i], "true", 4) == 0 || strncmp(&json_string[temp_i], "false", 5) == 0) {
                strcat(error_text, "cerialize ERROR: Multiple boolean values not allowed.\n");
                *failure = TRUE;
                return FALSE;
            }
            return FALSE;
        } else {
            strcat(error_text, "cerialize ERROR: Unexpected characters after 'false'.\n");
            *failure = TRUE;
            return FALSE;
        }
    } else {
        strcat(error_text, "cerialize ERROR: Expected 'true' or 'false'.\n");
        *failure = TRUE;
        return FALSE; // default return value
    }
}

static inline json_object parse_json_object(const char* json_string, cereal_size_t length, cereal_uint_t* i, char* error_text, bool_t* failure);

static inline json_list json_parse_list(const char* json_string, cereal_size_t length, cereal_uint_t* i, bool_t* failure, char* error_text) {
    skip_whitespace(json_string, length, i);

    if (json_string[*i] != LEX_OPEN_SQUARE) {
        strcat(error_text, "cerialize ERROR: Expected opening square '[' for JSON list.\n");
        *failure = TRUE;
        return (json_list){0, NULL}; // return empty list on error
    }
    (*i)++; // move past '['

    // count number of elements in the list
    cereal_size_t count = 0;
    bool_t found_closing_square = FALSE;
    json_object* list = NULL;
    while (*i < length) {
        skip_whitespace(json_string, length, i);
        char cur = json_string[*i];
        if (cur == LEX_CLOSE_SQUARE) {
            (*i)++; // move past ']'
            found_closing_square = TRUE;
            break; // end of list
        }

        // json_object* value = malloc(sizeof(json_object));
        json_object value = parse_json_object(json_string, length, i, error_text, failure);
        if (*failure) {
            strcat(error_text, "cerialize ERROR: Failed to parse value in JSON list.\n");
            return (json_list){0, NULL};
        }
        // add value to list
        list = realloc(list, sizeof(json_object) * (count + 1));
        if (list == NULL) {
            strcat(error_text, "cerialize ERROR: Failed to allocate memory for JSON list.\n");
            *failure = TRUE;
            return (json_list){0, NULL};
        }
        list[count] = value;
        count++;

        skip_whitespace(json_string, length, i);
    
        if (json_string[*i] != LEX_COMMA && json_string[*i] != LEX_CLOSE_SQUARE && *i != length) {
            strcat(error_text, "cerialize ERROR: Expected ',' or ']' after value in JSON list.\n");
            *failure = TRUE;
            return (json_list){0, NULL};
        }

        if (json_string[*i] == LEX_COMMA) {
            (*i)++; // move past ','
        }

        if (json_string[*i] == LEX_CLOSE_SQUARE) {
            found_closing_square = TRUE;
            (*i)++; // move past ']'
            break;
        }

        // skip whitespace before next item
        skip_whitespace(json_string, length, i);
    }

    // If we never found a closing square, this is an error
    if (!found_closing_square) {
        strcat(error_text, "cerialize ERROR: Expected closing square ']' for JSON list.\n");
        *failure = TRUE;
        if (list) free(list);
        return (json_list){0, NULL}; // return NULL on error
    }

    json_list result = {
        .count = count,
        .items = list // assign the array of json_object
    };
    return result;
}

static inline json_object parse_json_object(const char* json_string, cereal_size_t length, cereal_uint_t* i, char* error_text, bool_t* failure) {
    skip_whitespace(json_string, length, i);

    json_object obj;
    // obj.type = JSON_OBJECT;

    char cur = json_string[*i];
    if (cur == LEX_QUOTE) {
        obj.value.string = json_parse_string(json_string, length, i,failure, error_text);
        obj.type = JSON_STRING;
        return obj;
    }

    if (is_number_start(cur) || cur == LEX_PERIOD) {
        obj.value.number = json_parse_number(json_string, length, i, failure, error_text);
        obj.type = JSON_NUMBER;
        return obj;
    }

    if (cur == LEX_N) {
        obj.value.is_null = json_parse_null(json_string, (cereal_uint_t*)i, failure, error_text);
        obj.type = JSON_NULL;
        return obj;
    }

    if (cur == LEX_T || cur == LEX_F) {
        obj.value.boolean = json_parse_boolean(json_string, (cereal_uint_t*)i,failure, error_text);
        obj.type = JSON_BOOL;
        return obj;
    }
    
    if (cur == LEX_OPEN_SQUARE) {
        obj.value.list = json_parse_list(json_string, length, i, failure, error_text);
        obj.type = JSON_LIST;
        return obj;
    }

    // parse build object
    if (cur != LEX_OPEN_BRACE) {
        strcat(error_text, "cerialize ERROR: Expected opening brace '{' for JSON object.\n");
        *failure = TRUE;
        return (json_object){0}; // return empty value on error
    }
    (*i)++; // move past '{'

    skip_whitespace(json_string, length, i);

    json_node* head = NULL;
    cereal_size_t node_count = 0;

    // parse key-value pairs
    bool_t found_closing_brace = FALSE;
    while (*i < length) {
        skip_whitespace(json_string, length, i);
        cur = json_string[*i];
        if (json_string[*i] == LEX_CLOSE_BRACE) {
            (*i)++; // move past '}'
            found_closing_brace = TRUE;
            break; // end of object
        }

        char* key = json_parse_string(json_string, length, i, failure, error_text);
        if (key == NULL) {
            strcat(error_text, "cerialize ERROR: Failed to parse key in JSON object.\n");
            *failure = TRUE;
            return (json_object){0}; // return empty value on error
        }

        skip_whitespace(json_string, length, i);
        if (json_string[*i] != LEX_COLON) {
            strcat(error_text, "cerialize ERROR: Expected ':' after key in JSON object.\n");
            *failure = TRUE;
            free(key);
            return (json_object){0}; // return empty value on error
        }
        (*i)++; // move past ':'

        skip_whitespace(json_string, length, i);

        json_object value = parse_json_object(json_string, length, i, error_text, failure);
        if (*failure) {
            strcat(error_text, "cerialize ERROR: Failed to parse value in JSON object.\n");
            free(key);
            return (json_object){0}; // return empty value on error
        }

        // create new node
        json_node* new_node = (json_node*)malloc(sizeof(json_node));
        new_node->key = key;
        new_node->value = value; // copy the value

        node_count++;
        head = realloc(head, sizeof(json_node) * node_count);
        if (head == NULL) {
            strcat(error_text, "cerialize ERROR: Failed to allocate memory for JSON object.\n");
            *failure = TRUE;
            free(key);
            return (json_object){0}; // return empty value on error
        }
        head[node_count - 1] = *new_node;
        free(new_node);

        skip_whitespace(json_string, length, i);
        cur = json_string[*i];
        bool_t is_valid_delimiter = (cur == LEX_COMMA || cur == LEX_CLOSE_BRACE || cur == LEX_CLOSE_SQUARE || *i == length);
        if (!is_valid_delimiter) {
            strcat(error_text, "cerialize ERROR: Expected ',' or '}' after key-value pair in JSON object.\n");
            *failure = TRUE;
            free(key);
            return (json_object){0}; // return empty value on error
        }
        if (json_string[*i] == LEX_CLOSE_BRACE) {
            (*i)++; // move past '}'
            found_closing_brace = TRUE;
            break;
        }

        skip_whitespace(json_string, length, i);

        if (json_string[*i] == LEX_COMMA) {
            (*i)++; // move past ','
        }

        skip_whitespace(json_string, length, i);
    }

    // If we never found a closing brace, this is an error
    if (!found_closing_brace) {
        strcat(error_text, "cerialize ERROR: Expected closing brace '}' for JSON object.\n");
        *failure = TRUE;
        if (head) free(head);
        return (json_object){0};
    }

    // create the json_object
    obj.value.object.nodes = head; // assign the linked list of nodes
    obj.value.object.node_count = node_count; // store the number of nodes
    obj.type = JSON_OBJECT;

    return obj;
}

// parse json
static inline json deserialize_json(const char* json_string, cereal_size_t length) {

    bool_t failure = FALSE;
    char* error_text = malloc(JSON_MAX_ERROR_LENGTH);
    if (error_text == NULL) {
        json result = {
            .root = {0},
            .failure = TRUE,
            .error_text = "cerialize ERROR: Failed to allocate memory for error text.\n",
            .error_length = strlen("cerialize ERROR: Failed to allocate memory for error text.\n")
        };
        return result;
    }

    // TODO: parse json object
    cereal_uint_t i = 0;
    json_object root_value = parse_json_object(json_string, length, &i, error_text, &failure);

    json result = {
        .root = root_value,
        .failure = failure,
        .error_text = error_text
    };

    return result;
}


static inline char* serialize_string(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* result = (char*)malloc(len + 1);
    if (!result) return NULL;
    strcpy(result, str);
    return result;
}

static inline char* serialize_null() {
    char* result = (char*)malloc(5);
    if (!result) return NULL;
    strcpy(result, "null");
    return result;
}

static inline char* serialize_number(float number) { 
    char* result = malloc(32);
    if (!result) return NULL; // handle memory allocation failure
    snprintf(result, 32, "%f", number);
    return result;
}

static inline char* serialize_bool(bool_t value) {
    const char* src = value ? "true" : "false";
    char* result = (char*)malloc(strlen(src) + 1);
    if (!result) return NULL;
    strcpy(result, src);
    return result;
}

static inline char* serialize_list(json_list list) { 
    char* result = malloc(JSON_MAX_STRING_LENGTH);
    if (!result) return NULL; // handle memory allocation failure
    strcpy(result, "[");
    for (cereal_uint_t i = 0; i < list.count; i++) {
        // serialize each item in the list
        json item = {
            .root = list.items[i],
            .failure = 0,
            .error_text = NULL
        };
        char* item_str = serialize_json(&item);
        if (item_str) {
            strcat(result, item_str);
            free(item_str);
        }
        if (i < list.count - 1) {
            strcat(result, ",");
        }
    }
    strcat(result, "]");
    return result;
}

static inline char* serialize_object(const json_object* obj) { 
    char* result = malloc(JSON_MAX_STRING_LENGTH);
    if (!result) return NULL; // handle memory allocation failure
    // Defensive: handle empty or malformed objects
    if (obj->value.object.node_count == 0) {
        strcpy(result, "{}");
        return result;
    }

    strcpy(result, "{");
    for (cereal_size_t i = 0; i < obj->value.object.node_count; i++) {
        json_node node = obj->value.object.nodes[i];
        char* key_str = serialize_string(node.key);
        json value = {
            .root = { .type = node.value.type, .value = node.value.value },
            .failure = 0,
            .error_text = NULL
        };
        char* value_str = serialize_json(&value);
        if (key_str && value_str) {
            strcat(result, key_str);
            strcat(result, ":");
            strcat(result, value_str);
            free(key_str);
            free(value_str);
        }
        if (i < obj->value.object.node_count - 1) {
            strcat(result, ",");
        }
    }
    strcat(result, "}");
    return result;
}

static inline char* serialize_json(const json* j) { 
    if (!j) return NULL;

    json_object root = j->root;
    switch (root.type) {
        case JSON_STRING:
            return serialize_string(root.value.string);
        case JSON_NUMBER:
            return serialize_number(root.value.number);
        case JSON_BOOL:
            return serialize_bool(root.value.boolean);
        case JSON_NULL:
            return serialize_null();
        case JSON_LIST:
            return serialize_list(root.value.list);
        case JSON_OBJECT:
            return serialize_object(&root);
        default:
            return NULL;
    }
}

static inline json_object json_get_property(json_object obj, const char* key) {
    for (cereal_size_t i = 0; i < obj.value.object.node_count; i++) {
        json_node node = obj.value.object.nodes[i];
        if (strcmp(node.key, key) == 0) {
            return node.value;
        }
    }
    return (json_object){ .type = JSON_NULL };
}

// Memory management function implementations
static inline void json_object_free(json_object* obj) {
    if (!obj) return;
    
    switch (obj->type) {
        case JSON_STRING:
            if (obj->value.string) {
                free(obj->value.string);
                obj->value.string = NULL;
            }
            break;
            
        case JSON_LIST:
            // Free each item in the list
            for (cereal_size_t i = 0; i < obj->value.list.count; i++) {
                json_object_free(&obj->value.list.items[i]);
            }
            // Free the items array
            if (obj->value.list.items) {
                free(obj->value.list.items);
                obj->value.list.items = NULL;
            }
            obj->value.list.count = 0;
            break;
            
        case JSON_OBJECT:
            // Free each node
            for (cereal_size_t i = 0; i < obj->value.object.node_count; i++) {
                // Free the key
                if (obj->value.object.nodes[i].key) {
                    free(obj->value.object.nodes[i].key);
                    obj->value.object.nodes[i].key = NULL;
                }
                // Recursively free the value
                json_object_free(&obj->value.object.nodes[i].value);
            }
            // Free the nodes array
            if (obj->value.object.nodes) {
                free(obj->value.object.nodes);
                obj->value.object.nodes = NULL;
            }
            obj->value.object.node_count = 0;
            break;
            
        case JSON_NUMBER:
        case JSON_BOOL:
        case JSON_NULL:
            // No dynamic memory to free
            break;
    }
    
    // Reset the object type
    obj->type = JSON_NULL;
}

static inline void json_free(json* j) {
    if (!j) return;
    
    // Free error text if allocated
    if (j->error_text) {
        free(j->error_text);
        j->error_text = NULL;
    }
    
    // Free the root object
    json_object_free(&j->root);
    
    // Reset the structure
    j->error_length = 0;
    j->failure = FALSE;
}

#endif
