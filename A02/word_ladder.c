//
// AED, November 2022 (Tomás Oliveira e Silva)
//
// Second practical assignement (speed run)
//
// Place your student numbers and names here
//   N.Mec. 105937  Name: Bárbara Nóbrega Galiza
//   N.Mec. 109018  Name: Tomás António de Oliveira Victal
//
// Do as much as you can
//   1) MANDATORY: complete the hash table code
//      *) hash_table_create
//      *) hash_table_grow
//      *) hash_table_free
//      *) find_word
//      +) add code to get some statistical data about the hash table
//   2) HIGHLY RECOMMENDED: build the graph (including union-find data) -- use the similar_words function...
//      *) find_representative
//      *) add_edge
//   3) RECOMMENDED: implement breadth-first search in the graph
//      *) breadh_first_search
//   4) RECOMMENDED: list all words belonginh to a connected component
//      *) breadh_first_search
//      *) list_connected_component
//   5) RECOMMENDED: find the shortest path between to words
//      *) breadh_first_search
//      *) path_finder
//      *) test the smallest path from bem to mal
//         [ 0] bem
//         [ 1] tem
//         [ 2] teu
//         [ 3] meu
//         [ 4] mau
//         [ 5] mal
//      *) find other interesting word ladders
//   6) OPTIONAL: compute the diameter of a connected component and list the longest word chain
//      *) breadh_first_search
//      *) connected_component_diameter
//   7) OPTIONAL: print some statistics about the graph
//      *) graph_info
//   8) OPTIONAL: test for memory leaks
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../P02/elapsed_time.h"


//
// static configuration
//

#define _max_word_size_  32

typedef struct adjacency_node_s  adjacency_node_t;
typedef struct hash_table_node_s hash_table_node_t;
typedef struct hash_table_s      hash_table_t;
typedef struct queue_node        queue_node;
typedef struct queue_l           queue_l;

struct adjacency_node_s
{
  adjacency_node_t *next;            // link to th enext adjacency list node
  hash_table_node_t *vertex;         // the other vertex
};

struct hash_table_node_s
{
  // the hash table data
  char word[_max_word_size_];        // the word
  hash_table_node_t *next;           // next hash table linked list node
  // the vertex data
  adjacency_node_t *head;            // head of the linked list of adjancency edges
  int visited;                       // visited status (while not in use, keep it at 0)
  hash_table_node_t *previous;       // breadth-first search parent
  // the union find data
  hash_table_node_t *representative; // the representative of the connected component this vertex belongs to
  int number_of_vertices;            // number of vertices of the conected component (only correct for the representative of each connected component)
  int number_of_edges;               // number of edges of the conected component (only correct for the representative of each connected component)
};

struct hash_table_s
{
  unsigned int hash_table_size;      // the size of the hash table array
  unsigned int number_of_entries;    // the number of entries in the hash table
  unsigned int number_of_edges;      // number of edges (for information purposes only)
  hash_table_node_t **heads;         // the heads of the linked lists
};

//QUEUE-struct -----------------------------------------------------
struct queue_node{
  hash_table_node_t *hash_node;
  queue_node *next;
};
struct queue_l{
  unsigned int size;
  queue_node *head;
  queue_node *tail;
};

//QUEUE-funct -----------------------------------------------------
static queue_l *initialize_queue(){
  queue_l *queue = calloc(1,sizeof(queue_l));
  queue->head = NULL;
  queue->size = 0u;
  queue->tail = NULL;
  return queue;
}

static queue_node *allocate_queue_node(){
  queue_node *node = calloc(1,sizeof(queue_node));
  node->next = NULL;
  node->hash_node = NULL;
  return node;
}

static void put_node_queue(queue_l *queue,hash_table_node_t *hash_node){
  queue->size++;
  queue_node *q_node = allocate_queue_node();
  q_node->hash_node = hash_node;
  if(queue->size != 1){
  queue->tail->next = q_node;
  queue->tail = q_node;
  }else{
    queue->head = q_node;
    queue->tail = q_node;
  }
}

static void delete_queue(queue_l *queue){
  queue_node *node,*prev_node;
  node = queue->head;
  while(node != NULL){
    prev_node = node;
    node = node->next;
    free(prev_node);
  }
  free(queue);
}

static void print_queue_items(queue_l *queue){
  printf("\n");
  for(queue_node *node = queue->head;node != NULL;node = node->next){
    hash_table_node_t *hash_node = node->hash_node;
    printf("%s-",hash_node->word);
  };
}
//-----------------------------------------------------------------------------------


//
// data structures (SUGGESTION --- you may do it in a different way)
//



// declaration of functions
static hash_table_t *hash_table_create(void);
static void hash_table_grow(hash_table_t *hash_table);
static void hash_table_free(hash_table_t *hash_table);
static hash_table_node_t *find_word(hash_table_t *hash_table,const char *word,int insert_if_not_found);

//
// allocation and deallocation of linked list nodes (done)
//

static adjacency_node_t *allocate_adjacency_node(void)
{
  adjacency_node_t *node;

  node = (adjacency_node_t *)malloc(sizeof(adjacency_node_t));
  if(node == NULL)
  {
    fprintf(stderr,"allocate_adjacency_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_adjacency_node(adjacency_node_t *node)
{
  free(node);
}

static hash_table_node_t *allocate_hash_table_node(void)
{
  hash_table_node_t *node;

  node = (hash_table_node_t *)malloc(sizeof(hash_table_node_t));
  if(node == NULL)
  {
    fprintf(stderr,"allocate_hash_table_node: out of memory\n");
    exit(1);
  }
  return node;
}

static void free_hash_table_node(hash_table_node_t *node)
{
  free(node);
}

//
// hash table stuff (mostly to be done)
//

unsigned int crc32(const char *str)
{
  static unsigned int table[256];
  unsigned int crc;

  if(table[1] == 0u) // do we need to initialize the table[] array? no, bc its static and therefore initializated with 0u by default
  {
    unsigned int i,j;

    for(i = 0u;i < 256u;i++)
      for(table[i] = i,j = 0u;j < 8u;j++)
        if(table[i] & 1u)
          table[i] = (table[i] >> 1) ^ 0xAED00022u; // "magic" constant
        else
          table[i] >>= 1;
  }
  crc = 0xAED02022u; // initial value (chosen arbitrarily)
  while(*str != '\0')
    crc = (crc >> 8) ^ table[crc & 0xFFu] ^ ((unsigned int)*str++ << 24);
  return crc;
}

static hash_table_t *hash_table_create(void)
{
  hash_table_t *hash_table;
  unsigned int i;

  hash_table = (hash_table_t *)malloc(sizeof(hash_table_t));
  if(hash_table == NULL)
  {
    fprintf(stderr,"create_hash_table: out of memory\n");
    exit(1);
  }
  //
  // complete this
  //
  hash_table->hash_table_size = 100u;
  hash_table->number_of_entries = 0u;
  hash_table->number_of_edges = 0u;
  hash_table->heads = (hash_table_node_t **)calloc(hash_table->hash_table_size, sizeof(hash_table_node_t*));

  for(i = 0u; i < hash_table->hash_table_size; i++)
    hash_table->heads[i] = NULL;
  return hash_table;
}

static void hash_table_grow(hash_table_t *hash_table)
{
  //
  // complete this
  //
  unsigned int new_index;
  hash_table_node_t *node,*next_node,*new_node_spot;
  unsigned int new_size = hash_table->hash_table_size * 2;
  hash_table_node_t **new_heads = (hash_table_node_t **)calloc(new_size,sizeof(hash_table_node_t*));
  for(unsigned int i = 0u; i < new_size; i++){
    new_heads[i]= NULL;
  }

  for(unsigned int i = 0u; i < hash_table->hash_table_size; i++){
    node = hash_table->heads[i];
    while(node != NULL){
      next_node = node->next;
      node->next = NULL;
      new_index = crc32(node->word) % new_size;
      new_node_spot = new_heads[new_index];
      if(new_node_spot == NULL){
        new_heads[new_index] = node;
      }else{
        while(new_node_spot->next != NULL){
          new_node_spot = new_node_spot->next;
        }
        new_node_spot->next = node;
      }
      node = next_node;
    }
  }
  free(hash_table->heads);
  hash_table->heads = new_heads;
  hash_table->hash_table_size = new_size;
}

static void hash_table_free(hash_table_t *hash_table)
{
  //
  // complete this
  //
  for (unsigned int i=0u; i<hash_table->hash_table_size; i++) {
      hash_table_node_t *node = hash_table->heads[i];
      hash_table_node_t *previousNode;
      adjacency_node_t *adjNode;
      adjacency_node_t *previousAdjNode;
      while (node != NULL){
        adjNode = node->head;
        while(adjNode != NULL){
          previousAdjNode = adjNode;
          adjNode = adjNode->next;
          free_adjacency_node(previousAdjNode);
        }

        previousNode = node;
        node = node->next;

        free_hash_table_node(previousNode);
      }
    }
  free(hash_table->heads);
  free(hash_table);
}

// variables to get statistical data about the hash table
unsigned int number_of_collisions = 0u;
unsigned int max_linked_list_size = 0u;
double elapsed_time;

static hash_table_node_t *find_word(hash_table_t *hash_table,const char *word,int insert_if_not_found)
{
  hash_table_node_t *node;
  unsigned int i;

  i = crc32(word) % hash_table->hash_table_size;
  //
  // complete this
  //
  hash_table_node_t *previous_node;
  node = hash_table->heads[i];
  unsigned int size_linked_list = 0u;
  // find node
  while(node != NULL){
    if(strcmp(word,node->word) == 0) return node;
    if(node->next == NULL) previous_node = node; // save last non-null node 
    node = node->next;
    size_linked_list++;
  }
  if(insert_if_not_found){
    // create new node
    hash_table_node_t *new_node = allocate_hash_table_node();
    new_node->head = NULL;
    new_node->next = NULL;
    new_node->previous = NULL;
    new_node->visited = 0;
    new_node->representative = new_node;
    strcpy(new_node->word,word);
    // link node to hash table
    if(hash_table->heads[i] == NULL){
      hash_table->heads[i] = new_node;
    } 
    else{
      previous_node->next = new_node;
      number_of_collisions++;
    }
    // update current linked list size
    size_linked_list++;
    // updtae number of entries
    hash_table->number_of_entries++;
  }
  // save max linked list size
  if(size_linked_list > max_linked_list_size) max_linked_list_size = size_linked_list;
  // resize hash table
  if(hash_table->number_of_entries >= hash_table->hash_table_size*0.7) hash_table_grow(hash_table);
  return node;
}


void print_hash_table_statistics(hash_table_t *hash_table)
{
  printf("\n\n--------------------- Hash Table Statistical Data -------------------------\n");
  printf("\nnumber of entries: %u",hash_table->number_of_entries);
  printf("\nsize: %u", hash_table->hash_table_size);
  printf("\nnumber of collisions: %u", number_of_collisions);
  printf("\nmax linked list size: %u", max_linked_list_size);
  printf("\nelapsed time dispended on adding all file elements to hash table: %.4f\n", elapsed_time);
  printf("\n----------------------------------------------------------------------------\n");
}


//
// add edges to the word ladder graph (mostly do be done)
//

static hash_table_node_t *find_representative(hash_table_node_t *node)
{
  hash_table_node_t *representative,*next_node;

  //
  // complete this
  //
  return representative;
}

static void add_edge(hash_table_t *hash_table,hash_table_node_t *from,const char *word)
{
  hash_table_node_t *to,*from_representative,*to_representative;
  adjacency_node_t *link;

  to = find_word(hash_table,word,0);
  //
  // complete this
  //
  if(to != NULL){
    // add link to node "from" 
    link = allocate_adjacency_node();
    link->next = NULL;
    link->vertex = to;
    adjacency_node_t *from_links = from->head;
    if(from_links == NULL){
      from->head = link;
    } 
    else{
      while(from_links->next != NULL)
        from_links = from_links->next;
      from_links->next = link;
    }
    from->number_of_vertices++;
    // add link to node "to" 
    link = allocate_adjacency_node();
    link->next = NULL;
    link->vertex = from;
    adjacency_node_t *to_links = to->head;
    if(to_links == NULL){
      to->head = link;
    } 
    else{
      while(to_links->next != NULL)
        to_links = to_links->next;
      to_links->next = link;
    }
    to->number_of_vertices++;

    // increment total number of edges
    hash_table->number_of_edges++;

    from_representative = find_representative(from);
    to_representative = find_representative(to);
    // union
    if(from_representative != to_representative){
      if(to_representative->number_of_vertices > from_representative->number_of_vertices){
        from->representative = to_representative;
        to->representative->number_of_vertices += from_representative->number_of_vertices;
        to->representative->number_of_edges += from_representative->number_of_edges;
      }
      else{
        to->representative = from->representative;
        from->representative->number_of_vertices += to_representative->number_of_vertices;
        from->representative->number_of_edges += to_representative->number_of_edges;
      }
    }
  }
}


//
// generates a list of similar words and calls the function add_edge for each one (done)
//
// man utf8 for details on the uft8 encoding
//

static void break_utf8_string(const char *word,int *individual_characters)
{
  int byte0,byte1;

  while(*word != '\0')
  {
    byte0 = (int)(*(word++)) & 0xFF;
    if(byte0 < 0x80)
      *(individual_characters++) = byte0; // plain ASCII character
    else
    {
      byte1 = (int)(*(word++)) & 0xFF;
      if((byte0 & 0b11100000) != 0b11000000 || (byte1 & 0b11000000) != 0b10000000)
      {
        fprintf(stderr,"break_utf8_string: unexpected UFT-8 character\n");
        exit(1);
      }
      *(individual_characters++) = ((byte0 & 0b00011111) << 6) | (byte1 & 0b00111111); // utf8 -> unicode
    }
  }
  *individual_characters = 0; // mark the end!
}

static void make_utf8_string(const int *individual_characters,char word[_max_word_size_])
{
  int code;

  while(*individual_characters != 0)
  {
    code = *(individual_characters++);
    if(code < 0x80)
      *(word++) = (char)code;
    else if(code < (1 << 11))
    { // unicode -> utf8
      *(word++) = 0b11000000 | (code >> 6);
      *(word++) = 0b10000000 | (code & 0b00111111);
    }
    else
    {
      fprintf(stderr,"make_utf8_string: unexpected UFT-8 character\n");
      exit(1);
    }
  }
  *word = '\0';  // mark the end
}

static void similar_words(hash_table_t *hash_table,hash_table_node_t *from)
{
  static const int valid_characters[] =
  { // unicode!
    0x2D,                                                                       // -
    0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,           // A B C D E F G H I J K L M
    0x4E,0x4F,0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,           // N O P Q R S T U V W X Y Z
    0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,           // a b c d e f g h i j k l m
    0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,           // n o p q r s t u v w x y z
    0xC1,0xC2,0xC9,0xCD,0xD3,0xDA,                                              // Á Â É Í Ó Ú
    0xE0,0xE1,0xE2,0xE3,0xE7,0xE8,0xE9,0xEA,0xED,0xEE,0xF3,0xF4,0xF5,0xFA,0xFC, // à á â ã ç è é ê í î ó ô õ ú ü
    0
  };
  int i,j,k,individual_characters[_max_word_size_];
  char new_word[2 * _max_word_size_];

  break_utf8_string(from->word,individual_characters);
  for(i = 0;individual_characters[i] != 0;i++)
  {
    k = individual_characters[i];
    for(j = 0;valid_characters[j] != 0;j++)
    {
      individual_characters[i] = valid_characters[j];
      make_utf8_string(individual_characters,new_word);
      // avoid duplicate cases
      if(strcmp(new_word,from->word) > 0)
        add_edge(hash_table,from,new_word);
    }
    individual_characters[i] = k;
  }
}


//
// breadth-first search (to be done)
//
// returns the number of vertices visited; if the last one is goal, following the previous links gives the shortest path between goal and origin
//
/*
static int breadh_first_search(int maximum_number_of_vertices,hash_table_node_t **list_of_vertices,hash_table_node_t *origin,hash_table_node_t *goal)
{
  //
  // complete this
  //
  return -1;
}
*/
static int bfs(queue_l *queue,hash_table_node_t *origin,hash_table_node_t *goal){
  for(adjacency_node_t *node = origin->head;node != NULL;node = node->next){
    hash_table_node_t *vertex = node->vertex;
    if(vertex->visited == 1){
      printf("passed-%s\n",vertex->word);
      continue;
    }else{
    vertex->visited = 1;
    vertex->previous = origin;
    put_node_queue(queue,vertex);
    if(vertex == goal) return 1;
    }
  }
  return 0;
}
static void bfs_reset(hash_table_t *hash_table){
  hash_table_node_t *node;
  unsigned int i;
  for(i = 0u;i < hash_table->hash_table_size;i++)
    for(node = hash_table->heads[i];node != NULL;node = node->next){
      node->previous = NULL;
      node->visited = 0;
    }

}
/*
//
// list all vertices belonging to a connected component (complete this)
//

static void list_connected_component(hash_table_t *hash_table,const char *word)
{
  //
  // complete this
  //
}


//
// compute the diameter of a connected component (optional)
//

static int largest_diameter;
static hash_table_node_t **largest_diameter_example;

static int connected_component_diameter(hash_table_node_t *node)
{
  int diameter;

  //
  // complete this
  //
  return diameter;
}*/


//
// find the shortest path from a given word to another given word (to be done)
//

static void path_finder(hash_table_t *hash_table,const char *from_word,const char *to_word)
{
  //
  // complete this
  //
  hash_table_node_t *origin,*goal;
  origin = find_word(hash_table,from_word,0);
  if(origin == NULL) return;
  origin->visited = 1;
  goal = find_word(hash_table,to_word,0);
  if(goal == NULL) return;
  queue_l *queue = initialize_queue();
  put_node_queue(queue,origin);
  for(queue_node *node = queue->head;node != NULL;node = node->next){
    hash_table_node_t *hash_node = node->hash_node;
    int aux = bfs(queue,hash_node,goal);
    if(aux == 1) break;
  };
  printf("\n");
  //print_queue_items(queue);
  for(goal; goal != NULL;goal = goal->previous){
    printf("%s<-",goal->word);
  }
  bfs_reset(hash_table);
  delete_queue(queue);
}


//
// some graph information (optional)
//
/*
static void graph_info(hash_table_t *hash_table)
{
  //
  // complete this
  //
}
 */

//
// main program
//

int main(int argc,char **argv)
{
  char word[100],from[100],to[100];
  hash_table_t *hash_table;
  hash_table_node_t *node;
  unsigned int i;
  int command;
  FILE *fp;

  // initialize hash table
  hash_table = hash_table_create();
  // read words
  fp = fopen((argc < 2) ? "wordlist-four-letters.txt" : argv[1],"rb");
  if(fp == NULL)
  {
    fprintf(stderr,"main: unable to open the words file\n");
    exit(1);
  }
  elapsed_time = cpu_time();
  while(fscanf(fp,"%99s",word) == 1)
    (void)find_word(hash_table,word,1);
  elapsed_time = cpu_time() - elapsed_time;
  fclose(fp);

  // testando hash table -------------------
  printf("\n\n\n");
  int c = 0;
  for(i = 0u; i < hash_table->hash_table_size; i++){
    
    if(hash_table->heads[i] != NULL){
      //printf("%d", c++);
      hash_table_node_t *node = hash_table->heads[i];
      printf("%s %d", node->word, ++c);
      node = node->next;
      while(node != NULL){
        printf("%s %d", node->word, ++c);
        node = node->next;
      }
    }
  }

  print_hash_table_statistics(hash_table);

  // find all similar words
  for(i = 0u;i < hash_table->hash_table_size;i++)
    for(node = hash_table->heads[i];node != NULL;node = node->next)
      similar_words(hash_table,node);

  // testando add edge
  adjacency_node_t *link;
  for(i = 0u;i < hash_table->hash_table_size;i++)
    for(node = hash_table->heads[i];node != NULL;node = node->next){
      printf("\n\n\n %s", node->word);
      for(link = node->head; link != NULL; link = link->next)
        printf("\n%s", link->vertex->word);
    }

  path_finder(hash_table,"moço","paço");

  /* graph_info(hash_table);
  // ask what to do
  for(;;)
  {
    fprintf(stderr,"Your wish is my command:\n");
    fprintf(stderr,"  1 WORD       (list the connected component WORD belongs to)\n");
    fprintf(stderr,"  2 FROM TO    (list the shortest path from FROM to TO)\n");
    fprintf(stderr,"  3            (terminate)\n");
    fprintf(stderr,"> ");
    if(scanf("%99s",word) != 1)
      break;
    command = atoi(word);
    if(command == 1)
    {
      if(scanf("%99s",word) != 1)
        break;
      list_connected_component(hash_table,word);
    }
    else if(command == 2)
    {
      if(scanf("%99s",from) != 1)
        break;
      if(scanf("%99s",to) != 1)
        break;
      path_finder(hash_table,from,to);
    }
    else if(command == 3)
      break;
  } */
  // clean up
  hash_table_free(hash_table);
  return 0;
}
