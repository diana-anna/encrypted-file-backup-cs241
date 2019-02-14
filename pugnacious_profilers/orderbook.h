/**
* pugnacious_profilers Lab
* CS 241 - Spring 2019
*/

#include <stdio.h>

/* enum for Side*/
typedef enum o_side { BUY, SELL } OrderSide;

typedef struct onode {
    long id;
    char symbol[5];
    OrderSide side;
    long quantity;
    double price;
    struct onode *prev;
    struct onode *next;
} OrderNode;

/****Start Input Definitions*****/

/*
 * enum for input type
 * ASCII: Input is ASCII
 */
typedef enum i_type { ASCII } InputType;

/* struct to catch input*/
typedef struct order {
    int id;
    OrderSide side;
    int quantity;
    double price;
} Order;

/*
 * enum to distinguish between different struct types
 * LL: Struct for linked list implementation
 */

typedef enum s_type { LL } StructType;

/****Start Function implementations****/

/*
 * proc_scmdline - Processes the command line
 * argc - number of arguments
 * argv - arguments passed to the program command line
 * i_t - capture the input type
 * s_t - structure type: linked list
 * in - input file
 * out - output file
 */
void proc_cmdline(int argc, char *argv[], InputType *i_t, StructType *s_t,
                  FILE **in, int *out);

/*
 * add_order_LL - add order to linked list
 * head - head of the linked list
 * data - node data
 */
void add_order_LL(OrderNode **head, OrderNode *data);

/*
 * cancel_order_LL - cancel order from linked list
 * head - head of the linked list
 * data - node data
 */
void cancel_order_LL(OrderNode **head, OrderNode *data);

/*
 * reduce_quantity_LL - reduce quantity in a linked list
 * head - head of the linked list
 * data - node data
 */
void reduce_quantity_LL(OrderNode *head, OrderNode *data);

/*
 * change_order_LL - change the date of an order
 * head - head of the linked list
 * data - node data
 */
void change_order_LL(OrderNode *head, OrderNode *data);

/*
 * proc_mesgs_LL - Process a file into a linked list
 * in_type - Tyoe of input (ASCII)
 * head - head of the linked list
 * in - inputfile
 */
void proc_mesgs_LL(InputType in_type, OrderNode **head, FILE *in);

/*
 * print_orderbook_LL - Print the order book
 * head - head of the linked list
 * out - outputfile
 */
void print_orderbook_LL(OrderNode *head, int out);

/*
 * free_LL - free the entire list
 * head - head of the linked list
 */
void free_LL(OrderNode **head);

// End Linked List Implementation
