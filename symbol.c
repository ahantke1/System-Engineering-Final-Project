#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "Debug.h"
#include "symbol.h"

// @author Andy Hantke
// Created as a final project for Computer Systems Class

/** size of LC3 memory 65,536 Addresses */
#define LC3_MEMORY_SIZE  (1 << 16)

/** Provide prototype for strdup() */
char *strdup(const char *s);

/** defines data structure used to store nodes in hash table */
typedef struct node {
  struct node* next;     /**< linked list of symbols at same index */
  int          hash;     /**< hash value - makes searching faster  */
  symbol_t     symbol;   /**< the data the user is interested in   */
} node_t;

/** defines the data structure for the hash table */
struct sym_table {
  int      capacity;    /**< length of hast_table array                  */
  int      size;        /**< number of symbols (may exceed capacity)     */
  node_t** hash_table;  /**< array of head of linked list for this index */
  char**   addr_table;  /**< look up symbols by addr                     */
};

/** djb hash - found at http://www.cse.yorku.ca/~oz/hash.html
 * tolower() call to make case insensitive.
 */

static int symbol_hash (const char* name) {
  unsigned char* str  = (unsigned char*) name;
  unsigned long  hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + tolower(c); /* hash * 33 + c */

  c = hash & 0x7FFFFFFF; /* keep 31 bits - avoid negative values */

  return c;
}

// Creating the table object
sym_table_t* symbol_init (int capacity) {
  sym_table_t *x = calloc(1 ,sizeof(sym_table_t));            // Allocate memory for symbol table 
  x->capacity = capacity;                                    // set the capacity of the created symbol table to the passed capacity
  x->size = 0;                                              // initializing the initial size to 0
  x->hash_table = calloc(capacity, sizeof(node_t*));       // Initializing the Hash Table
  x->addr_table = calloc(LC3_MEMORY_SIZE, sizeof(char*)); // Initializing the Addr Table
  return x;                                              // Returns the pointer to the Symbol Table
}

// The 'desturctor'
void symbol_term (sym_table_t* symTab) {
  symbol_reset(symTab);           // Freeing the Symbols and Nodes
  free(symTab->hash_table);      // Freeing the Hash Table
  free(symTab->addr_table);     // Freeing the Addr Table
  free(symTab);                // Freeing the Symbol Table
}

// Reset the table
void symbol_reset(sym_table_t* symTab) {
 for(int i = 0; i < symTab->capacity; i++){  // For every Symbol
    if (symTab->hash_table[i] != NULL) { // If the hashtable at the index exists
    node_t *tmp = symTab->hash_table[i];  // Go to a different index
      while(tmp != NULL) {               // While theres still a Hash Table at the index
        symTab->hash_table[i] = NULL;   // set the pointer to null
        symbol_t *accessedSym = &(tmp->symbol);
        free(accessedSym->name);     // Free the symbol from memory
        node_t *tmp1 = tmp->next;   // Save the address to the next node
        free(tmp);                 //  Free the node
        tmp = tmp1;               // Push the next node into tmp
      }   
    }
  }
}

// Add a value to the table
int symbol_add (sym_table_t* symTab, const char* name, int addr) {
  int hash = 0;                                           // initializing hash
  int index = 0;                                         // initializing index
  if (symbol_search(symTab,name,&hash,&index) != NULL){ // Checking to see if Symbol already exists
    return 0;                                          // if yes return 0
  }  
  node_t *tmp = calloc(1, sizeof(node_t));           // Temp variable
  tmp->symbol.name = strdup(name);                  // Setting the Name of the New Symbol
  tmp->symbol.addr = addr;                         // Setting the Address of the New Symbol
  tmp->hash = hash;                               // Setting the node hash value at the calculated index of the symbol 
  tmp->next = symTab->hash_table[index];         // Udpate the next value 
  symTab->hash_table[index] = tmp;              // Sets tmp as head
  if (symTab->addr_table){
    symTab->addr_table[addr] = tmp->symbol.name;
  }
  symTab->size++;
  return 1;
}

// Search for a symbol
struct node* symbol_search (sym_table_t* symTab, const char* name, int* hash, int* index) {
  if (symTab == NULL) return NULL;      // Make sure the table isn't empty
  *hash = symbol_hash(name);           // Getting the Hash Value of Name
  *index = *hash % symTab->capacity;  // Calculating the Index 
  node_t *tmp = symTab->hash_table[*index]; // Temp variable
  if (tmp == NULL) return NULL;            // Make sure the node exists
  while(tmp){                             // While theres still more hash table
    if(*hash == (tmp->hash)){            // if calculated hash is in the hash table
      if (strcasecmp(name, (tmp->symbol.name)) == 0 ){ // string comparison
        return (tmp);                                 // if true return the symbol in the table
        }
      }
      tmp = tmp->next; // Update to the next node
    }
  return NULL; // else return NULL
}

// Search for a symbol by name
symbol_t* symbol_find_by_name (sym_table_t* symTab, const char* name) {
  int hash = 0;                                                  // initializing hash
  int index = 0;                                                // initializing index
  node_t* foundNode = symbol_search(symTab,name,&hash,&index); // Searching for the Corrisponding Symb Node
  if (foundNode == NULL){                                     // if the node is empty
    return NULL;                                                  
  }
  symbol_t* foundSymb = &foundNode->symbol;   // Get the address of symbol from that node
  return foundSymb;
}

// Search for a symbol by mem address
char* symbol_find_by_addr (sym_table_t* symTab, int addr) {
  if (symTab){                              // if we have a symbol table
    if (symTab->addr_table){               // that has an address table
      return symTab->addr_table[addr];    // get the value of the address table at the address
    }
  }
 return NULL;
}

// Iterate helper-function 
void symbol_iterate (sym_table_t* symTab, iterate_fnc_t fnc, void* data) {
  for(int i = 0; i < symTab->capacity; i++){        // For every Symbol
    if (symTab->hash_table[i] != NULL) {           // if the hash table at index i is not null
    node_t *tmp = symTab->hash_table[i];          // Access the underlying LinkedList
      while(tmp != NULL) {                       // While theres still a Hash Table at the index
        symbol_t *accessedSym = &(tmp->symbol); // 
        (*fnc)(accessedSym, data);
        tmp = tmp->next; // Update to the next node
      }  
    }
  }
}

// Size helper funciton
int symbol_size (sym_table_t* symTab) {
  return symTab->size;
}

// Comparison helper function
int compare_names (const void* vp1, const void* vp2) {
  symbol_t* sym1 = *((symbol_t**) vp1); // Sets the Type of vp1 for comparison
  symbol_t* sym2 = *((symbol_t**) vp2); // Sets the Type of vp2 for comparison
  return strcasecmp(sym1->name, sym2->name); // Compares the names and returns based on the comparison
}

// Comparison helper function
int compare_addresses (const void* vp1, const void* vp2) {
  symbol_t* sym1 = *((symbol_t**) vp1); // Sets the Type of vp1 for comparison
  symbol_t* sym2 = *((symbol_t**) vp2); // Sets the Type of vp2 for comparison
  if ((sym1->addr) - (sym2->addr) == 0) { // If the address' are the same
     return compare_names(vp1,vp2); // Using Name as secondary sort key
   }
   return (sym1->addr) - (sym2->addr); // Return the difference in addresses
}

// Ordering/Organizing algo
symbol_t** symbol_order (sym_table_t* symTab, int order) {
  symbol_t** srtSymb = calloc(symTab->size, sizeof(symbol_t)); // Allocating space for sorted array
  int srtSymbIndex = 0; // tracking the # of added symbols
    for(int i = 0; i < symTab->capacity; i++){  // For every Symbol
      if (symTab->hash_table[i] != NULL) {
      node_t *tmp = symTab->hash_table[i];  // Go to a different index
        while(tmp != NULL) { // While theres still a Hash Table at the index
          symbol_t *accessedSym = &(tmp->symbol); // gets the pointer symbol 
          srtSymb[srtSymbIndex] = accessedSym; // puts the symbol in the array
          srtSymbIndex++; // increment the index
          tmp = tmp->next; // Update to the next node
        }
      }  
    } 
  if (order == HASH) {
    qsort(srtSymb, (symTab->size), sizeof(symbol_t*),*compare_addresses); //qsort
  }
  if (order == NAME){
    qsort(srtSymb, (symTab->size), sizeof(symbol_t*),compare_names); //qsort
  }
  if (order == ADDR){
    qsort(srtSymb, (symTab->size), sizeof(symbol_t*),*compare_addresses); //qsort
  }
  return NULL;
}

