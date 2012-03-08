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

// Pack a size and allocated bit into a word
#define PACK(size, alloc) ((size) | (alloc))

// Read and write a word at address p
#define GET(p)	(*((unsigned int *) (p)))
#define PUT(p, val) (*((unsigned int *) (p)) = ((unsigned int) val))

//Read the size and allocated fields from address p
#define GET_SIZE(p) 	(GET(p) & ~0x7)
#define GET_ALLOC(p) 	(GET(p) & 0x1)

// Given block ptr bp, compute address of its header and footer
#define HDRP(bp) 	((char *)(bp) - WSIZE)
#define FTRP(bp) 	((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE)

//Give block ptr bp, compute address of next and previous blocks
#define NEXT_BLKP(bp) 	((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE)))
#define PREV_BLKP(bp)	((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE)))


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
#define SET_RB(p, rb)           ((p != NULL) ? PUT( MC(p)-WSIZE, PACK_T( GET_SIZE_T(p), GET_ALLOC(p), rb ) ):(unsigned int)p)
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


//private global pointers
//void *heap_listp;
static void *root;  //base node of RBTree
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
	//Create the initial empty heap
	/*if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1)	return -1;

	PUT(heap_listp, 0);				// Alignment padding
	PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1));	// Prologue header
	PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1));	// Prologue footer
	PUT(heap_listp + (3*WSIZE), PACK(0, 1));	// Epilogue header
	heap_listp += (2*WSIZE);

	//Extend the empty heap with a free block of CHUNKSIZE bytes
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;*/

	if ((dumby = mem_sbrk(4*WSIZE)) == (void *)-1) return -1;
	SET_SIZE(dumby, 0);
	SET_RB(dumby, BLACK);
	SET_ALLOC(dumby, 1);
	SET_LEFT(dumby, NULL);
	SET_RIGHT(dumby, NULL);
	SET_PARENT(dumby, NULL);
	heap_size += 4;
	
	if (extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;

	return 0;
}



/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{

	size_t asize; 		// Adjusted block size
	size_t extendsize; 	// Amount to extend heap if no fit
	char *bp;

	// Ignore spurious requests
	if (size == 0)
		return NULL;
	
	// Adjust block size to include overhead and alignment reqs
	if (size <= DSIZE)
		asize = DSIZE;
	else
		asize = ALIGN(size);//DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

	// Search the free list for a fit
	if ((bp = (char*)rem_delete(asize)) != NULL) {//if ((bp = find_fit(asize)) != NULL) {
		if (GET_SIZE(bp) > asize) 
			split(bp, asize);		
		SET_ALLOC(bp, 1);
		return bp;
	}

	// No fit found. Get more memory and place the block
	extendsize = MAX(asize+4, CHUNKSIZE);
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	if(CHUNKSIZE > asize+4) {
		split(bp, asize);
		SET_ALLOC(bp, 1);
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

/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
	void *oldptr = ptr;
	void *newptr;
	size_t copySize;
	copySize = GET_SIZE_T(oldptr);    //*(size_t *)((char *)oldptr - SIZE_T_SIZE);
	if(size == copySize) return ptr;

	newptr = mm_malloc(size);
	if(oldptr == NULL)
		return newptr;
	if(size == 0) {
		mm_free(oldptr);
		return NULL;
	}

	if (newptr == NULL)
		return NULL;

	if (size < copySize)
	copySize = size;
	memcpy(newptr, oldptr, copySize);
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
	heap_size += (size / WSIZE);

	//Init free block header/footer and the epilogue header
	bp += 12;
	SET_SIZE(bp, size-(4*WSIZE));		//Set size of base node
	insert((int*)bp);
	return coalesce(bp);
}


static void *coalesce(void * bp) {  // search tree for coalesce, HOW DO YOU DO THAT WITH THE TREE???
	size_t prev_alloc = GET_ALLOC_T(GET_LAST(bp)); 	//GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC_T(GET_NEXT(bp));	//GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE_T(bp);			//GET_SIZE(HDRP(bp));
	int* next = GET_NEXT(bp);
	int* last = GET_LAST(bp);

	if((unsigned int)next >= ((unsigned int)(dumby - 3)) + heap_size) next_alloc = 1;

	if (prev_alloc && next_alloc) { //CASE 1
		return bp;
	}

	else if (prev_alloc && !next_alloc) { //CASE 2
		size += GET_SIZE_T(next) + 4;	//GET_SIZE(HDRP(NEXT_BLKP(bp)));
		delete(bp);
		delete(next);
		SET_SIZE(bp, size);
		insert(bp);
		//PUT(HDRP(bp), PACK(size, 0));
		//PUT(FTRP(bp), PACK(size, 0));
	}

	else if (!prev_alloc && next_alloc) { //CASE 3
		/*size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);*/
		size += GET_SIZE_T(last) + 4;
		delete(last);
		delete(bp);
		bp = last;
		SET_SIZE(bp, size);
		insert(bp);
	}

	else { //CASE 4
		/*size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);*/
		size += GET_SIZE_T(next) + GET_SIZE_T(last) + 8;
		delete(last);
		delete(bp);
		delete(next);
		bp = last;
		SET_SIZE(bp, size);
		insert(bp);
	}
	return bp;
}

void split(char* node, size_t size)
{	int size_b = GET_SIZE(node) - size - 4;
	SET_SIZE(node, size);
	int* next_node = GET_NEXT(node);
	SET_SIZE(next_node, size_b);
	insert(next_node);
	return;
}

int * get_last(int * bp) {

	int* addr = (int*) (MC(bp)-4*WSIZE);

	if (GET_NEXT(root) == bp)
		return root;

	if (addr == 0) {
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

int* find(int nsize) {
  int* current = root;

  while(1) {
    if(nsize <= GET_SIZE_T(current)) { //try to go left
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
  int size = GET_SIZE_T(node);

  SET_LEFT(node, NULL);
  SET_RIGHT(node, NULL);
  SET_SIZE(node, size);
  SET_RB(node, RED);
  SET_ALLOC(node, 0);
  SET_PARENT(node, NULL);

  if(num_nodes == 0) {
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

  return 0;
}

void icase1(int * node) {
  if(GET_PARENT(node) == NULL) {
    SET_RB(node, BLACK);
  } else {
    icase2(node);
  }
  SET_RB(root, BLACK);
}

void icase2(int * node) {
  int* parent = GET_PARENT(node);
  if(GET_RB(parent) == BLACK)
    return;
  else icase3(node);
}

void icase3(int * node) {
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
}

void icase4(int * node) {
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
}

void icase5(int * node) {
  int* p = GET_PARENT(node);
  if(p == NULL) return;
  int* gp = GET_PARENT(p);

  SET_RB(p, BLACK);
  SET_RB(gp, RED);
  if( node == GET_LEFT(p) ) rotate_clock(gp);
  else rotate_counter_clock(gp);
}

int* rem_find(int nsize) {
	int* current = root;
	int* closest_seen = NULL;


	while(1) {
		if(nsize == GET_SIZE_T(current)) return current;
		if(nsize < GET_SIZE_T(current)) { //try to go left
			if(GET_LEFT(current) == NULL) { // if there's no node to the left, it's the parent
				break;
			} else {
				if(closest_seen == NULL) {
					closest_seen = current;
				}
				if((GET_SIZE_T(closest_seen) > nsize) && (GET_SIZE_T(closest_seen) >= GET_SIZE_T(current))) closest_seen = current;
				current = (int*)GET_LEFT(current); // go left and continue
				continue;
			}
		} else { // try to go right
			if(GET_RIGHT(current) == NULL) { // if there's no node to the right, it's the parent
				break;
			} else {
				if((GET_SIZE_T(closest_seen) > nsize) && (GET_SIZE_T(closest_seen) >= GET_SIZE_T(current))) closest_seen = current;
				current = (int*)GET_RIGHT(current); // go left and continue
			}
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

int* getSucPre(int* node) {
  int* temp;

  if ((temp = GET_LEFT(node)) != NULL) {
    while(GET_RIGHT(temp) != NULL) {
      temp = GET_RIGHT(temp);
    }
    return temp;
  } else if ((temp = GET_RIGHT(node)) != NULL) {
    while(GET_LEFT(temp) != NULL) {
      temp = GET_LEFT(temp);
    }
    return temp;
  } else {
    return NULL;
  }
} 

int* get_sibling(int* node) {
  int* parent = GET_PARENT(node);
  if(parent == 0) return NULL;

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





