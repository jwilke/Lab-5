/*struct lnode {
	int *data;
	struct lnode * next;
	};*/
/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>
#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your team information in the following struct.
 ********************************************************/
team_t team = {
    /* Team name */
    "jlw3599-grambo",
    /* First member's full name */
    "Jacob Wilke",
    /* First member's email address */
    "jake.wilke@gmail.com",
    /* Second member's full name (leave blank if none) */
    "Graham Benevelli",
    /* Second member's email address (leave blank if none) */
    "grahambenevelli@gmail.com"
};

#define WSIZE 4			// Word and header/footer size (in bytes)
#define DSIZE 8			// Double word size (in bytes)
#define CHUNKSIZE (1<<12)	// Extend heap by this amount (in bytes)

#define MAX(x, y) ((x) > (y) ? (x) : (y))

// Read and write a word at address p
#define GET(p)	(*((unsigned int *) (p)))
#define PUT(p, val) (*((unsigned int *) (p)) = ((unsigned int) val))


/* single word (4) or double word (8) alignment */
#define ALIGNMENT 8

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x7)


#define SIZE_T_SIZE (ALIGN(sizeof(size_t)))




/* Node macros */
#define MC(p)			((char*) (p))
#define SET_LEFT(p, addr)	((p != NULL) ? PUT(MC(p)-(3*WSIZE), addr):(unsigned int)p)
#define SET_RIGHT(p, addr)	((p != NULL) ? PUT(MC(p)-(2*WSIZE), addr):(unsigned int)p)
#define SET_SIZE(p, size) 	((p != NULL) ? PUT(MC(p)-WSIZE, PACK_T(size, GET_ALLOC_T(p), GET_RB(p))):(unsigned int)p)
#define SET_PARENT(p, addr)	((p != NULL) ? PUT(MC(p)+GET_SIZE_T(p), addr):(unsigned int)p)
#define SET_ALLOC(p, alloc)	((p != NULL) ? PUT( MC(p)-WSIZE, PACK_T( GET_SIZE_T(p), alloc, GET_RB(p) ) ):-1)
#define SET_RB(p, rb)           ((p != NULL) ? PUT( MC(p)-WSIZE, PACK_T( GET_SIZE_T(p), GET_ALLOC_T(p), rb ) ):(unsigned int)p)

#define PACK_T(size, alloc, RB) ((size) | (!!((unsigned int) alloc)) | ((!!((unsigned int) RB))<<1))

#define GET_LEFT(p)		((p != NULL) ? ( (int *) GET(MC(p)-3*(WSIZE)) ):NULL)
#define GET_RIGHT(p)		((p != NULL) ? ( (int *) GET(MC(p)-2*(WSIZE)) ):NULL)
#define GET_ALLOC_T(p)		((p != NULL) ? (GET(MC(p)-1*(WSIZE)) & 0x1):-1)
#define GET_SIZE_T(p)		((p != NULL) ? (GET(MC(p)-1*(WSIZE)) & ~0x3):(unsigned int)p)
#define GET_RB(p)		((p != NULL) ? ((GET(MC(p)-1*(WSIZE)) & 0x2) >> 1):0)
#define GET_PARENT(p)		((p != NULL) ? ( (int *) GET(MC(p)+GET_SIZE_T(p)) ):NULL)
#define GET_LAST(p)		get_last(p)
#define GET_NEXT(p)		( (int *) ((MC(p)+4*WSIZE+GET_SIZE_T(p))) )

#define OVERHEAD 16

#define BLACK 0
#define RED   1

//void printTree();
//void print_tree_level(struct lnode* olist);
void print_node(int * node);
void print_order();


//private global pointers
//void *heap_listp;
static void *root = NULL;  //base node of RBTree
static int num_nodes = 0;
static int * dumby = NULL;  //first node of the heap. used for removal in rbtree
static int heap_size = 0;

//the project functions
int mm_init(void);
void *mm_malloc(size_t size);
void mm_free(void *ptr);
void *mm_realloc(void *ptr, size_t size);

//the project helper functions
static void *extend_heap(size_t words);
static void *coalesce(void *bp);
int * get_last(int * bp);
void split(char* node, size_t size);

//tester functions
int mm_check(void);
int allFreeBlocks(int* current);
int checkCoalescing(int* current);
int freeInTree(int* current);
int validHeapAddr(int* current);
int testRoot(int* r);
int testOrdering(int* r);
int testRedNodes(int* r);
int testBlackPaths(int* r);

//insert functions
int* find(int nsize); // find the parent that leaf of size nsize would have
int insert(int * node);
void icase1(int * node);
void icase2(int * node);
void icase3(int * node);
void icase4(int * node);
void icase5(int * node);

//delete functions
int* rem_find(int nsize);
int* delete(int *node);
int* rem_delete(int size);
int* delete_sub(int * n);
void dcase1(int * n);
void dcase2(int * n);
void dcase3(int * n);
void dcase4(int * n);
void dcase5(int * n);
void dcase6(int * n);

//general-use tree functions
void rotate_clock(int * node);
void rotate_counter_clock(int * node);
int* createNode(int * base, int size);
int* getSucPre(int* node);
int* get_sibling(int* node);
void swap_nodes(int* a, int* b);
int* replace_node_one_child(int* node);

/* 
 * mm_init - initialize the malloc package.
 */
int mm_init(void)
{

	root = NULL;

	if ((dumby = mem_sbrk(9*WSIZE)) == (void *)-1) return -1;

	dumby += 8;
	SET_SIZE(dumby, 0);
	SET_RB(dumby, BLACK);
	SET_ALLOC(dumby, 1);
	SET_LEFT(dumby, NULL);
	SET_RIGHT(dumby, NULL);
	SET_PARENT(dumby, NULL);
	heap_size = 9;
	num_nodes = 0;

	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;

	return 1;
}



/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
	
	size_t asize; 		// Adjusted block size
	//size_t extendsize; 	// Amount to extend heap if no fit
	char *bp;

	// Ignore spurious requests
	if (size == 0)
		return NULL;
	
	// Adjust block size to include overhead and alignment reqs
	asize = ALIGN(size); // always works.  no need for an if statement
	// Search the free list for a fit
	if ((bp = (char*)rem_delete(asize)) != NULL) {
		SET_ALLOC(bp, 1);

		if (GET_SIZE_T(bp) > asize + 16) 
			split(bp, asize);

		return bp;
	}

	// No fit found. Get more memory and place the block
	if ((bp = extend_heap(MAX(asize+16, CHUNKSIZE)/WSIZE)) == NULL)
		return NULL;

	delete((int*) bp);
	SET_ALLOC(bp, 1);

	if(GET_SIZE_T(bp) > asize+16) {
		split(bp, asize);
	}

	return bp;
}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{
	SET_ALLOC(ptr, 0);
	insert(ptr);
	coalesce(ptr);
}

void adjust_node(int* node, int dec_size) {
  int size = GET_SIZE_T(node) - dec_size;
  delete(node);
  node += (dec_size/WSIZE);
  SET_SIZE(node, size);
  insert(node);
}


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	void *oldptr = ptr;
	void *newptr;
	size_t oldSize;

	if(size == 0) {
		mm_free(oldptr);
		return NULL;
	}
	if(ptr == NULL) {
	  ptr = mm_malloc(size);
	  return ptr;
	}


	oldSize = GET_SIZE_T(oldptr);
	if(size == oldSize) return ptr;

	if(size > oldSize) {
	        int* next = GET_NEXT(ptr);
		int asize = ALIGN(size);
      	        if((next < (dumby - 8) + heap_size) && GET_ALLOC_T(next) == 0 && oldSize + GET_SIZE_T(next) - 16 > asize) {
		        //then next node can be adjusted
	                //printf("current size: %d/t size needed: %d/t next size: %d\n", copySize, size, GET_SIZE_T(next));
		  adjust_node(next, (asize - oldSize));
		  SET_SIZE(ptr, asize);
		  SET_PARENT(ptr, NULL);
		  return ptr;
	        }
        } /*else {
	  //We are shrinking the node
	  int asize = ALIGN(size);
	  if(oldSize - asize >= 8 + 16) {
	    split(ptr, asize);
	    return ptr;
	  }
	  }*/

	//Last resort, malloc new memory and copy old data to new pointer
	newptr = mm_malloc(size);

	if (newptr == NULL)
		return NULL;

	if (size < oldSize) {
	        oldSize = size;
        }
	memcpy(newptr, oldptr, oldSize);
	mm_free(oldptr);
	return newptr;
}

static void *extend_heap(size_t words) {
	char *bp;
	size_t size;

	//Allocate an even number of words to maintaign alignment
	size = (words % 2) ? (words + 1) * WSIZE : words * WSIZE;
	if((long)(bp = mem_sbrk(size)) == -1)
		return NULL;
		
	heap_size += (size/WSIZE);

	bp += 3*WSIZE;

	//Init free block header/footer and the epilogue header
	SET_SIZE(bp, size-(4*WSIZE));
	SET_ALLOC(bp, 0);

	insert((int*)bp);

	return coalesce(bp);
}


static void *coalesce(void * bp) {
	size_t prev_alloc = GET_ALLOC_T(GET_LAST(bp));
	size_t next_alloc = GET_ALLOC_T(GET_NEXT(bp));
	size_t size = GET_SIZE_T(bp);
	int* next = GET_NEXT(bp);
	int* last = GET_LAST(bp);

	if(next >= (dumby - 8) + heap_size) next_alloc = 1;
	if(last < (dumby + 1)) prev_alloc = 1;


	if (prev_alloc && next_alloc) { //CASE 1
		return bp;
	}

	

	else if (prev_alloc && !next_alloc) { //CASE 2
		size += GET_SIZE_T(next) + 16;
		delete(bp);
		delete(next);
		SET_SIZE(bp, size);
		while( ((next = GET_NEXT(bp)) < (dumby - 8) + heap_size) && GET_ALLOC_T(next) == 0) {
		    size += GET_SIZE_T(next) + 16;
		    delete(next);
		    SET_SIZE(bp,size);
		}

		insert(bp);
		return bp;
	}

	else if (!prev_alloc && next_alloc) { //CASE 3
		size += GET_SIZE_T(last) + 16;
		delete(last);
		delete(bp);
		bp = last;
		SET_SIZE(bp, size);
		while( ((last = GET_LAST(bp)) > (dumby+1)) && GET_ALLOC_T(last) == 0) {
		  size += GET_SIZE_T(last) + 16;
		  delete(last);
		  bp = last;
		  SET_SIZE(bp, size);
		}
		insert(bp);
		return bp;
	}

	else { //CASE 4

		size += GET_SIZE_T(next) + GET_SIZE_T(last) + 32;
		delete(last);
		delete(bp);
		delete(next);
		bp = last;
		SET_SIZE(bp, size);
		insert(bp);
	}

	return coalesce(bp);
}

void split(char* node, size_t size)
{
	int size_b = GET_SIZE_T(node) - size - 4*WSIZE;

	SET_SIZE(node, size);
	SET_PARENT(node, NULL);
	SET_ALLOC(node, 1);

	int* next_node = GET_NEXT(node);
	SET_SIZE(next_node, size_b);
	SET_ALLOC(next_node, 0);

	insert(next_node);
	return;
}

int * get_last(int * bp) {

	int* addr = (int*) (MC(bp)-4*WSIZE);

	if (GET_NEXT(root) == bp)
		return root;

	if (addr == 0 || addr < dumby + 4) {
		return NULL;
	}

	int *last_parent = (int*) GET(addr);

	if (last_parent == 0) 
		return NULL;

	int *parent_left = GET_LEFT(last_parent);
	
	if (parent_left != NULL && GET_NEXT(parent_left) == bp) {
		return parent_left;
	}

	int *parent_right = GET_RIGHT(last_parent);

	if (parent_left != NULL && GET_NEXT(parent_right) == bp) {
		return parent_right;
	}

	//either allocated or root
	return NULL;
}



int* find(int nsize) {
  unsigned int ns = (unsigned int) nsize;
  int* current = root;
  //assert (root != NULL);
  while(1) {
    if(ns <= GET_SIZE_T(current)) { //try to go left
      if(GET_LEFT(current) == NULL) { // if there's no node to the left, it's the parent
	break;
      } else {
	current = (int*)GET_LEFT(current); // go left and continue
	continue;
      }
    } else { // try to go right
      if(GET_RIGHT(current) == NULL) { // if there's no node to the right, it's the parent
	break;
      } else {
	current = (int*)GET_RIGHT(current); // go left and continue
      }
    }

  }
  return current;
}

int insert(int * node) { //assumes header/footer is already created
  //assert(GET_SIZE_T(node) > 0);
  int size = GET_SIZE_T(node);

  SET_LEFT(node, NULL);
  SET_RIGHT(node, NULL);
  SET_SIZE(node, size);
  SET_RB(node, RED);
  SET_ALLOC(node, 0);
  SET_PARENT(node, NULL);

  if(root == NULL) {
    num_nodes++;
    root = node;
    SET_RB(root, BLACK);
    SET_PARENT(root, NULL);
    return 1;
  }
  num_nodes++;

  int* parent = find(size);
  if(size <= GET_SIZE_T(parent)) {
    SET_LEFT(parent,node);
    SET_PARENT(node, parent);
  } else {
    SET_RIGHT(parent, node);
    SET_PARENT(node, parent);
  }

  icase1(node);
  //assert(GET_ALLOC_T(node) == 0);
  return 0;
}

void icase1(int * node) {
  //assert(GET_ALLOC_T(node) == 0);
  if(GET_PARENT(node) == NULL) {
    SET_RB(node, BLACK);
  } else {
    icase2(node);
  }
  SET_RB(root, BLACK);
  //assert(GET_ALLOC_T(node) == 0);
}

void icase2(int * node) {
  //assert(GET_ALLOC_T(node) == 0);;
  int* parent = GET_PARENT(node);
  if(GET_RB(parent) == BLACK)
    return;
  else icase3(node);
  //assert(GET_ALLOC_T(node) == 0);
}

void icase3(int * node) {
  //assert(GET_ALLOC_T(node) == 0);
  int* parent = GET_PARENT(node);
  int* u = get_sibling(parent); //uncle

  if( (u != NULL) && (GET_RB(u) == RED)) {
    SET_RB(parent, BLACK); // set parent to black
    SET_RB(u, BLACK); // set uncle to black
    SET_RB(GET_PARENT(parent),RED); // set grandparent to red
    icase1(GET_PARENT(parent)); // check if grandparent is good
  } else {
    icase4(node);
  }
  //assert(GET_ALLOC_T(node) == 0);
}

void icase4(int * node) {
  //assert(GET_ALLOC_T(node) == 0);
  int* p = GET_PARENT(node);
  int* gp = GET_PARENT(p);

  if( (node == GET_RIGHT(p)) && (p == GET_LEFT(gp)) ) {
    rotate_counter_clock(p);
    node = GET_LEFT(node);
  } else if ( (node == GET_LEFT(p)) && (p == GET_RIGHT(gp)) ) {
    rotate_clock(p);
    node = GET_RIGHT(node);
  }
  icase5(node);
  //assert(GET_ALLOC_T(node) == 0);
}

void icase5(int * node) {
  //assert(GET_ALLOC_T(node) == 0);
  int* p = GET_PARENT(node);
  if(p == NULL) return;
  int* gp = GET_PARENT(p);

  SET_RB(p, BLACK);
  SET_RB(gp, RED);
  if( node == GET_LEFT(p) ) rotate_clock(gp);
  else rotate_counter_clock(gp);
  //assert(GET_ALLOC_T(node) == 0);
}

int* rem_find(int nsize) {
	int* current = root;
	int* closest_seen;
	if (GET_SIZE_T(root) >= nsize)
		closest_seen = root;
	else
		closest_seen = NULL;


	while(1) {
		if(nsize == GET_SIZE_T(current)) return current;
		if(nsize < GET_SIZE_T(current)) { 
			if(closest_seen == NULL) {
				closest_seen = current;
			}
			if((GET_SIZE_T(closest_seen) > nsize) && 
				(GET_SIZE_T(closest_seen) >= GET_SIZE_T(current))) 
				closest_seen = current;
				
			if(GET_LEFT(current) == NULL)
				break;

			current = (int*)GET_LEFT(current);

			continue;
			
		} else { 
			
			if(GET_RIGHT(current) == NULL) { 
				return closest_seen;
			}
			current = (int*)GET_RIGHT(current); 
			
		}

	}
	if( GET_SIZE_T(closest_seen) >= nsize ) return closest_seen;
	if(nsize > GET_SIZE_T(current)) return NULL;

	return NULL;
}

int* rem_delete(int size) {
	// find node to delete
	int * rem = rem_find(size);
	// delete node if it existed
	if(rem != NULL)
		delete(rem);
	// return node
	return rem;
}

int* delete(int *node) {
	if(node == NULL) return NULL;
	if(node == dumby) return NULL;

	num_nodes--;
	int* sorp = getSucPre(node);

	if(sorp != NULL) {
		swap_nodes(node, sorp);
	}

	delete_sub(node);
	//SET_ALLOC(node, 1);
	return node;
}




int* delete_sub(int* n) {
	int* copy = n;
	int dumbyUsed= 0;
	int* child = replace_node_one_child(n);

	if (child == NULL) {
		dumbyUsed = 1; 
		swap_nodes(n, dumby);
		child = dumby;
	}

	if(GET_RB(n) == BLACK) {
		if (GET_RB(child) == RED) {
			SET_RB(child, BLACK);
		} else {
			dcase1(child);
		}
	}

	if (dumbyUsed) {
		int* dp = GET_PARENT(dumby);
		if(dumby == GET_LEFT(dp)) {
			SET_LEFT(dp, NULL);
		} else {
			SET_RIGHT(dp, NULL);
		}
		SET_SIZE(dumby, 0);
		SET_RB(dumby, BLACK);
		SET_ALLOC(dumby, 1);
		SET_LEFT(dumby, NULL);
		SET_RIGHT(dumby, NULL);
		SET_PARENT(dumby, NULL);
		if (root == dumby) root = NULL;
	}

	return copy;
}

void dcase1(int * n) {
	if (GET_PARENT(n) != NULL) {
		dcase2(n);
	}
	
}

void dcase2(int * n) {
	int * s = get_sibling(n);

	if (GET_RB(s) == RED) {
		int * p = GET_PARENT(n);
		SET_RB(p, RED);
		SET_RB(s, BLACK);
		if(n == GET_LEFT(p))
			rotate_counter_clock(p);
		else 
			rotate_clock(p);
	}
	dcase3(n);
}

void dcase3(int * n) {
	int* s = get_sibling(n);
	int* p = GET_PARENT(n);
	if (GET_RB(p) == BLACK && GET_RB(s) == BLACK && 
		GET_RB(GET_LEFT(s)) == BLACK && GET_RB(GET_RIGHT(s)) == BLACK) {
		
		SET_RB(s, RED);
		dcase1(p);
	} else {
		dcase4(n);
	}

}

void dcase4(int* n) {
	int* s = get_sibling(n);
	int* p = GET_PARENT(n);

	if (GET_RB(p) == RED && GET_RB(s) == BLACK && 
		GET_RB(GET_LEFT(s)) == BLACK && GET_RB(GET_RIGHT(s)) == BLACK) {
		
		SET_RB(s, RED);
		SET_RB(p, BLACK);
	} else {
		dcase5(n);
	}

}

void dcase5(int* n) {
	int* s = get_sibling(n);
	int* p = GET_PARENT(n);

	if ( GET_RB(s) == BLACK ) {
		if( (n == GET_LEFT(p)) && (GET_RB(GET_RIGHT(s)) == BLACK) && (GET_RB(GET_LEFT(s)) == RED) ) {
			SET_RB(s, RED);
			SET_RB(GET_LEFT(s), BLACK);
			rotate_clock(s);
		} else if ((n == GET_RIGHT(p)) && (GET_RB(GET_LEFT(s)) == BLACK) && (GET_RB(GET_RIGHT(s))== RED)) {
			SET_RB(s, RED);
			SET_RB(GET_RIGHT(s), BLACK);
			rotate_counter_clock(s);
		}
	}

	dcase6(n);
}

void dcase6(int* n) {
	int* s = get_sibling(n);
	int* p = GET_PARENT(n);

	SET_RB(s, GET_RB(p));
	SET_RB(p, BLACK);

	if( n == GET_LEFT(p) ) {
		SET_RB(GET_RIGHT(s), BLACK);
		rotate_counter_clock(p);
	} else {
		SET_RB(GET_LEFT(s), BLACK);
		rotate_clock(p);
	}
}



void rotate_clock(int * node) {
	int* p = GET_PARENT(node);
	int* l = GET_LEFT(node);
	int* childs_r = GET_RIGHT(l);

	if(p != NULL) {
		if(GET_LEFT(p) == node) SET_LEFT(p, l);
		else SET_RIGHT(p, l);
		SET_PARENT(l, p);
	}

	SET_RIGHT(l,node);
	SET_PARENT(node, l);
	SET_LEFT(node, childs_r);
	if(childs_r != NULL)
		SET_PARENT(childs_r,node);
	if (root == node) {
		root = l;
		SET_PARENT(l, NULL);
	}
}

void rotate_counter_clock(int * node) {
	int* p = GET_PARENT(node);
	int* r = GET_RIGHT(node);
	int* childs_l = GET_LEFT(r);

	if(p != NULL) {
		if(GET_RIGHT(p) == node) SET_RIGHT(p, r);
		else SET_LEFT(p, r);
		SET_PARENT(r, p);
	}

	SET_LEFT(r,node);
	SET_PARENT(node,r);
	SET_RIGHT(node, childs_l);
	if(childs_l != NULL)
		SET_PARENT(childs_l, node);
	if (root == node) {
		root = r;
		SET_PARENT(r, NULL);
	}
}

int * createNode(int * base, int size) {
  SET_LEFT(base, NULL);
  SET_RIGHT(base, NULL);
  SET_SIZE(base, size);
  SET_ALLOC(base, 0);
  SET_RB(base, BLACK);
  SET_PARENT(base, NULL);
return base;
}


int* get_sibling(int* node) {
  int* parent = GET_PARENT(node);
  if(parent == NULL) return NULL;

  if((int*)GET_LEFT(parent) == node) {
    return GET_RIGHT(parent);
  }
  return GET_LEFT(parent);
}

void swap_nodes(int* a, int* b) { //assumes a is higher in the tree
	

	int ca = GET_RB(a);
	int* la = GET_LEFT(a);
	int* ra = GET_RIGHT(a);
	int* pa = GET_PARENT(a);
	int lopa = (a == GET_LEFT(pa));

	int cb = GET_RB(b);
	int* lb = GET_LEFT(b);
	int* rb = GET_RIGHT(b);
	int* pb = GET_PARENT(b);
	int lopb = (b == GET_LEFT(pb));

	SET_RB(a, cb);
	SET_RB(b, ca);

	SET_LEFT(a, lb);
	SET_PARENT(lb, a);
	SET_LEFT(b, la);
	SET_PARENT(la, b);

	SET_RIGHT(a, rb);
	SET_PARENT(rb, a);
	SET_RIGHT(b, ra);
	SET_PARENT(ra, b);

	SET_PARENT(a, pb);
	SET_PARENT(b, pa);

	if(lopb) {
		SET_LEFT(pb, a);
	} else {
		SET_RIGHT(pb, a);
	}

	if(lopa) {
		SET_LEFT(pa, b);
	} else {
		SET_RIGHT(pa, b);
	}

	if(pb == a) { //if the two nodes are connected
		if(la == b) {
			SET_LEFT(b, a);
			SET_LEFT(a, lb);
		} else {
			SET_RIGHT(b,a);
			SET_RIGHT(a, rb);
		}
		SET_PARENT(a,b);
	}

	if(root == a) root = b;
	else if(root == b) root = a;
}

int* getSucPre(int* node) {
	int * l = GET_LEFT(node);
	int * r = GET_RIGHT(node);
	int * temp = NULL;
	if (l != NULL) {
		temp = l;
		while(GET_RIGHT(temp) != NULL)
			temp = GET_RIGHT(temp);
	} else if (r != NULL) {
		temp = r;
		while(GET_LEFT(temp) != NULL)
			temp = GET_LEFT(temp);
	}
	return temp;
}

int* replace_node_one_child(int* node) {
	if(GET_RIGHT(node) == NULL && GET_LEFT(node) == NULL) return NULL;
	int* p = GET_PARENT(node);
	if( node == GET_LEFT(p) ) {
		SET_LEFT(p, GET_LEFT(node));
		SET_PARENT(GET_LEFT(node), p);
		return GET_LEFT(node);
	} else {
		SET_RIGHT(p, GET_RIGHT(node));
		SET_PARENT(GET_RIGHT(node), p);
	}
	return GET_RIGHT(node);
}



/*
void printTree() {
	printf("**********************\nroot\n");
  struct lnode* printTree = malloc(sizeof(struct lnode));
  printTree->data = root;
  printTree->next = NULL;
  print_tree_level(printTree);
	printf("***********************\n");
}

void print_tree_level(struct lnode* olist) {
  struct lnode* nlist = malloc(sizeof(struct lnode));
  nlist->data = NULL;
  nlist->next = NULL;
  struct lnode* cur = nlist;
  if(olist->data == NULL) return;
  struct lnode* temp;
  char color = 'R';

  while(olist != NULL) {
    if(GET_RB(olist->data) == 0) color = 'B';
    else color = 'R';

    printf("%p:%d:%d:%c ", olist->data, GET_ALLOC_T(olist->data), GET_SIZE_T(olist->data), color);
    if(GET_LEFT(olist->data) != NULL) {
      temp = malloc(sizeof(struct lnode));
      temp->data = GET_LEFT(olist->data);
      temp->next = NULL;
      nlist->next = temp;
      nlist = nlist->next;
    }
    if(GET_RIGHT(olist->data) != NULL) {
      temp = malloc(sizeof(struct lnode));
      temp->data = GET_RIGHT(olist->data);
      temp->next = NULL;
      nlist->next = temp;
      nlist = nlist->next;
    }
    struct lnode * f = olist->next;
    free(olist);
    olist = f;
  }
  if(cur->next != NULL && cur->data == NULL) {
    struct lnode * f = cur->next;
    free(cur);
    cur = f;
  }
  printf("\n");
  print_tree_level(cur);
}
*/




void print_node(int * node) {
	if (node == NULL) return;
  printf("CURRENT ADDRESS: %p\n", node);
  printf("NODE:\nLEFT: %p\n", GET_LEFT(node));
  printf("RIGHT: %p\n", GET_RIGHT(node));
  printf("SIZE: %d\n", GET_SIZE_T(node));
  printf("ALLOC: %d\n", GET_ALLOC_T(node));
  printf("RB: %d\n", GET_RB(node));
  printf("PARENT: %p\n\n", GET_PARENT(node));
  return;
}

int mm_check(void) {
  int allPassed = 0;
  allPassed = allPassed || allFreeBlocks(root);
  allPassed = allPassed || checkCoalescing(dumby);
  allPassed = allPassed || freeInTree(root);
  allPassed = allPassed || validHeapAddr(root);
  // check red black
  if(testRoot(root) == 0) allPassed = 1;
  if(testOrdering(root) == 0) allPassed = 1;
  if(testRedNodes(root) == 0) allPassed = 1;
  if(testBlackPaths(root) == -1) allPassed = 1;

  return allPassed;
}

// is every block in the tree free
int allFreeBlocks(int* current) {
  if (current == NULL) return 0; 
  else if (GET_ALLOC_T(current) == 1) return 1;
  else{
    int out = 0;
    out = out | allFreeBlocks(GET_LEFT(current));
    out = out | allFreeBlocks(GET_RIGHT(current));
    return out;
  }
}

// make sure there is no adjacent free blocks
int checkCoalescing(int* current) {
  if(current >= (dumby - 8) + heap_size) {
    return 0;
  }
  if(GET_NEXT(current) >= (dumby - 8) + heap_size) {
    return 0;
  } else {
    if(GET_ALLOC_T(current) == 1) 
      return checkCoalescing(GET_NEXT(current));
    int out = 0;
    if(GET_ALLOC_T(GET_NEXT(current)) == 0) out = 1;
    if(out == 0) out = checkCoalescing(GET_NEXT(current));
    
    return out;
  }
}

// make sure all free nodes are in tree
int freeInTree(int* current) {
  if(current >=(dumby - 8) + heap_size) {
    return 0;
  }
  
  if(GET_NEXT(current) >= (dumby - 8) + heap_size) {
    return 0;
  }

  if(GET_ALLOC_T(current) == 1) return freeInTree(GET_NEXT(current));

  int out = 1;
  int* p = GET_PARENT(current);
  if(GET_LEFT(p) == current) out = 0;
  if(GET_RIGHT(p) == current) out = 0;
  return out;
}


// make sure all addresses are in heap
int validHeapAddr(int* current){
  if(current == NULL) return 0;
  int out = 1;
  if(current <= (dumby - 8) + heap_size  && current > dumby) out = 0;
  if (out == 0) out = validHeapAddr(GET_LEFT(current));
  if (out == 0) out = validHeapAddr(GET_RIGHT(current));
  return out;
}

// make sure hte root is black
int testRoot(int* r) {
  if (r == NULL) return 1;
  else return (GET_RB(r) == BLACK);
}

// make sure the tree is in order
int testOrdering(int* r) {
  if (r == NULL) return 1;
  int out = 1;
  if (GET_LEFT(r) != NULL) out = out && GET_SIZE_T(r) >= GET_SIZE_T(GET_LEFT(r));
  if (GET_RIGHT(r) != NULL) out = out && GET_SIZE_T(r) <= GET_SIZE_T(GET_RIGHT(r));
  out = out && testOrdering(GET_LEFT(r));
  out = out && testOrdering(GET_RIGHT(r));
  return out;
}

// make sure that there are no red nodes with red children
int testRedNodes(int* r) {
  if (r == NULL) return 1;
  int out = 1;
  if (GET_RB(r) == RED) {
    if(GET_LEFT(r) != NULL)
      out = out && GET_RB( GET_LEFT(r) ) == BLACK;
    if(GET_RIGHT(r) != NULL)
      out = out && GET_RB( GET_RIGHT(r) ) == BLACK;
  }
  return 0;
}

// amek sure all paths have the same number of blacks
int testBlackPaths(int* r) {
  if (r == NULL) return 0;
  int l;
  int rgt;
  l = testBlackPaths(GET_LEFT(r));
  rgt = testBlackPaths(GET_RIGHT(r));
  if (l == -1 || rgt == -1) return -1;
  if (l == rgt) {
    if (GET_RB(r) == BLACK)
      return l+1;
    else
      return l;
  }
  return -1;
}





































































void printVerbose(int exp, int result) {
  	printf("expected: \t%d\ngot:\t\t%d\n", exp, result);
}

void printVerbosep(int* exp, int result) {
  	printf("expected: \t%p\ngot:\t\t%d\n", exp, result);
}
 
void unit(int verbose) {
	printf("\nTesting Nodes\n");
	char method[30];
	strcpy(method, "SET_LEFT()");
	int result;
	int allPassed = 1;
	int test = 1;

	int* bp = malloc(23*sizeof(int));
	int* left = bp+3;
	int* rootr = left+5;
	int * right = rootr+6;
	int* leaf = right+7;

	root = rootr;


	SET_LEFT(left, leaf);
	SET_LEFT(right, NULL);
	SET_LEFT(rootr, left);
	SET_LEFT(leaf, NULL);
	
	result = ((int*) bp[0]) == leaf;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = ((int*) bp[5]) == left;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = ((int*) bp[11]) == NULL;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = ((int*) bp[18]) == NULL;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;






	
	strcpy(method, "GET_LEFT()");

	result = GET_LEFT(left) == leaf;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = GET_LEFT(rootr) == left;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = GET_LEFT(right) == NULL;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = GET_LEFT(leaf) == NULL;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;



	strcpy(method, "SET_RIGHT()");

	SET_RIGHT(left, NULL);
	SET_RIGHT(right, NULL);
	SET_RIGHT(rootr, right);
	SET_RIGHT(leaf, NULL);

	result = bp[1] == 0;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = ((int*) bp[6]) == right;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = bp[12] == 0;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = bp[19] == 0;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;




	strcpy(method, "GET_RIGHT()");

	result = GET_RIGHT(left) == 0;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = GET_RIGHT(rootr) == right;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = GET_RIGHT(right) == 0;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = GET_RIGHT(leaf) == 0;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;




	strcpy(method, "SET_SIZE()");

	SET_SIZE(left, 1*WSIZE);
	SET_SIZE(right, 3*WSIZE);
	SET_SIZE(rootr, 2*WSIZE);
	SET_SIZE(leaf, 1*WSIZE);

	result = (bp[2] & ~0x3) == 4;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = (bp[7] & ~0x3) == 8;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = (bp[13] & ~0x3) == 12;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = (bp[20] & ~0x3) == 4;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;





	strcpy(method, "GET_SIZE_T()");

	result = (GET_SIZE_T(left)) == 4;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = GET_SIZE_T(rootr) == 8;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = GET_SIZE_T(right) == 12;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = GET_SIZE_T(leaf) == 4;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;



	strcpy(method, "GET_ALLOC_T()");

	bp[2] = (bp[2] & ~0x1) | (1);
	  result = (GET_ALLOC_T(left)) == 1;
        if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
	  allPassed = 0;
	  if(verbose) printVerbose(5, bp[2]);
        }
        test++;

	bp[7] =(bp[7] & ~0x1) | (1);
	  result = (GET_ALLOC_T(left)) == 1;
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(5, bp[2]);
        }
        test++;

	bp[20] =(bp[20] & ~0x1) | (0);
	  result = (GET_ALLOC_T(left)) == 1;
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(5, bp[2]);
        }
        test++;





        strcpy(method, "GET_RB()");

        bp[2] = (bp[2] & ~0x2) | (2);
	result = (GET_RB(left));
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
        }
        test++;

	bp[7] = (bp[7] & ~0x2) | (2);
	result = (GET_RB(rootr));
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(10, bp[7]);
        }
        test++;

	bp[13] = (bp[13] & ~0x2);
	result = !(GET_RB(right));
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(12, bp[13]);
        }
        test++;

	bp[2] = (bp[20] & ~0x2);
	result = !(GET_RB(leaf));
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(4, bp[20]);
        }
        test++;




	strcpy(method, "SET_RB()");

	SET_RB(left, 1);
	SET_RB(rootr, 1);
	SET_RB(right, 0);
	SET_RB(leaf, 0);

        result = (GET_RB(left));
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
        }
        test++;

	result = (GET_RB(rootr));
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
	}
        test++;

	result = !(GET_RB(right));
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
	}
        test++;

	result = !(GET_RB(leaf));
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
	}
        test++;

	strcpy(method, "SET_RB()");

        SET_ALLOC(left, 1);
        SET_ALLOC(rootr, 2);
        SET_ALLOC(right, 0);
        SET_ALLOC(leaf, 0);

        result = (GET_ALLOC_T(left));
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
        }
        test++;

	result = (GET_ALLOC_T(rootr));
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
	}
        test++;

        result = !(GET_ALLOC_T(right));
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
	  if(verbose) printVerbose(7, bp[2]);
	}
        test++;

	result = !(GET_ALLOC_T(leaf));
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
	}
        test++;


        strcpy(method, "SET_PARENT()");

        SET_PARENT(left, rootr);
        SET_PARENT(rootr, 0);
        SET_PARENT(right, rootr);
        SET_PARENT(leaf, left);

        result = ((int*) bp[4]) == rootr;
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
	  allPassed = 0;
	  if(verbose) printVerbosep(rootr, bp[4]);
        }
        test++;

        result = bp[10] == 0;
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(0, bp[10]);
        }
        test++;

        result = (bp[17] == (int)rootr);
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbosep(rootr, bp[17]);
        }
        test++;

        result = (bp[22] == (int)left);
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbosep(left, bp[22]);
        }
        test++;




	strcpy(method, "SET_PARENT()");

        result = (GET_PARENT(left)) == rootr;
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
        }
        test++;

	result = (GET_PARENT(rootr)) == 0;
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
        }
        test++;

	result = (GET_PARENT(right)) == rootr;
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
        }
        test++;

	result = (GET_PARENT(leaf)) == left;
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
        }
        test++;


        strcpy(method, "GET_NEXT()");

	result = GET_NEXT(left) == rootr;
	if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbosep(left, result);
        }
        test++;

	result = GET_NEXT(rootr) == right;
	if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbosep(right, (unsigned int)GET_NEXT(rootr));
        }
        test++;

	result = GET_NEXT(right) == leaf;
	if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbosep(leaf, (unsigned int)GET_NEXT(right));
        }
        test++;

        strcpy(method, "GET_LAST()");

	int * r = GET_LAST(rootr);
	result = r == left;
	if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbosep(left, (unsigned int)r);
        }
        test++;

	result = GET_LAST(right) == rootr;
	if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbosep(rootr, (unsigned int)GET_NEXT(right));
        }
        test++;

	result = GET_LAST(leaf) == right;
	if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbosep(leaf, (unsigned int)GET_NEXT(right));
        }
        test++;


	if(allPassed) {
		printf("All %d tests passed\n\n", test-1);
	}
	free(bp);
}

void print_order() {
	printf("***********************\n");
	int* temp = dumby;
	while(temp < (int*) mem_heap_hi()) {
		printf("%p:%d:%d\n", temp, GET_ALLOC_T(temp), GET_SIZE_T(temp));
		temp = GET_NEXT(temp);
	}
	printf("***********************\n");
}
