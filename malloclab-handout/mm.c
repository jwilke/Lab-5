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
    "Wilkevelli",
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
#define SET_LEFT(p, addr)	PUT(MC(p)-(3*WSIZE), addr)
#define SET_RIGHT(p, addr)	PUT(MC(p)-(2*WSIZE), addr)
#define SET_SIZE(p, size) 	PUT(MC(p)-WSIZE, PACK_T(size, GET_ALLOC_T(p), GET_RB(p)))
#define SET_PARENT(p, addr)	PUT(MC(p)+GET_SIZE_T(p), addr)
#define SET_ALLOC(p, alloc)	PUT( MC(p)-WSIZE, PACK_T( GET_SIZE_T(p), alloc, GET_RB(p) ) )
#define SET_RB(p, rb)           PUT( MC(p)-WSIZE, PACK_T( GET_SIZE_T(p), GET_ALLOC(p), rb ) )
#define PACK_T(size, alloc, RB) ((size) | (!!((unsigned int) alloc)) | ((!!((unsigned int) RB))<<1))

#define GET_LEFT(p)		GET(MC(p)-3*(WSIZE))
#define GET_RIGHT(p)		GET(MC(p)-2*(WSIZE))
#define GET_ALLOC_T(p)		(GET(MC(p)-1*(WSIZE)) & 0x1)
#define GET_SIZE_T(p)		(GET(MC(p)-1*(WSIZE)) & ~0x3)
#define GET_RB(p)		(GET(MC(p)-1*(WSIZE)) & 0x2)
#define GET_PARENT(p)		GET(MC(p)+GET_SIZE_T(p))
#define GET_LAST(p)		((GET_LEFT( GET(MC(p)-4*WSIZE)) == p ) ? (GET_LEFT(GET(MC(p)-4*WSIZE))) : (GET_RIGHT(GET(MC(p)-4*WSIZE))))
#define GET_NEXT(p)		((MC(p)+4*WSIZE+GET_SIZE_T(p)))


//private global pointers
static void *heap_listp;
static void *root;  //base node of RBTree

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

	if ((root = mem_sbrk(3*WSIZE)) == (void *)-1) return -1;
	
	PUT(root, 0);				//Alignment padding
	SET_LEFT(root, NULL);			//Set left pointer
	// PUT(root-(3*WSIZE), NULL)
	// (*((unsigned int *) (root-(3*WSIZE))) = (NULL))
	SET_RIGHT(root, NULL);			//Set right pointer
	SET_SIZE(root, 0);			//Set size of base node
	
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
	if (size ==0)
		return NULL;
	
	// Adjust block size to include overhead and alignment reqs
	if (size <= DSIZE)
		asize = 2*DSIZE;
	else
		asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);

	// Search the free list for a fit
	if ((bp = find_fit(asize)) != NULL) {
		place(bp, asize);
		return bp;
	}

	// No fit found. Get more memory and place the block
	extendsize = MAX(asize, CHUNKSIZE);
	if ((bp = extend_heap(extendsize/WSIZE)) == NULL)
		return NULL;
	place(bp, asize);
	return bp;

}


/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *ptr)
{

	size_t size = GET_SIZE(HDRP(ptr));
	
	PUT(HDRP(ptr), PACK(size, 0));
	PUT(FTRP(ptr), PACK(size, 0));
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
    
    newptr = mm_malloc(size);
    if (newptr == NULL)
      return NULL;
    copySize = GET_SIZE(HDRP(oldptr));    //*(size_t *)((char *)oldptr - SIZE_T_SIZE);
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

	//Init free block header/footer and the epilogue header
	//SET_RIGHT(root, bp); //add to tree
	SET_LEFT(bp, NULL);			//Set left pointer
	SET_RIGHT(bp, NULL);			//Set right pointer
	SET_SIZE(bp, size-(3*WSIZE));		//Set size of base node

	return coalesce(bp);
}


static void *coalesce(void * bp) {  // search tree for coalesce, HOW DO YOU DO THAT WITH THE TREE???
	size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
	size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
	size_t size = GET_SIZE(HDRP(bp));

	if (prev_alloc && next_alloc) { //CASE 1
		return bp;
	}

	else if (prev_alloc && !next_alloc) { //CASE 2
		size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp), PACK(size, 0));
		PUT(FTRP(bp), PACK(size, 0));
	}

	else if (!prev_alloc && next_alloc) { //CASE 3
		size += GET_SIZE(HDRP(PREV_BLKP(bp)));
		PUT(FTRP(bp), PACK(size, 0));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}

	else { //CASE 4
		size += GET_SIZE(HDRP(PREV_BLKP(bp))) + GET_SIZE(FTRP(NEXT_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
		PUT(FTRP(NEXT_BLKP(bp)), PACK(size, 0));
		bp = PREV_BLKP(bp);
	}
	return bp;
}

static void *find_fit(size_t asize) {
	char *current = heap_listp;
	for( ;GET_SIZE(HDRP(current)) > 0; current = NEXT_BLKP(current)) {
		if(!GET_ALLOC(HDRP(current)) && (asize <= GET_SIZE(HDRP(current)))) {
			return (void *)current;
		}
	}

	return NULL;
}

static void place (void *bp, size_t asize) {
	size_t csize = GET_SIZE(HDRP(bp));

	if ((csize - asize) >= (2*DSIZE)) {
		PUT(HDRP(bp), PACK(asize, 1));
		PUT(FTRP(bp), PACK(asize, 1));
		bp = NEXT_BLKP(bp);
		PUT(HDRP(bp), PACK(csize-asize, 0));
		PUT(FTRP(bp), PACK(csize-asize, 0));
	}
	else {
		PUT(HDRP(bp), PACK(csize, 1));
		PUT(FTRP(bp), PACK(csize, 1));
	}
} 

void printVerbose(int exp, int result) {
  printf("expected: \t%d\ngot:\t\t%d\n", exp, result);
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
	int* root = left+5;
	int * right = root+6;
	int* leaf = right+7;

	SET_LEFT(left, leaf);
	SET_LEFT(right, NULL);
	SET_LEFT(root, left);
	SET_LEFT(leaf, NULL);
	
	result = bp[0] == leaf;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = bp[5] == left;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = bp[11] == NULL;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = bp[18] == NULL;
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

	result = GET_LEFT(root) == left;
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
	SET_RIGHT(root, right);
	SET_RIGHT(leaf, NULL);

	result = bp[1] == 0;
	if (!result) {
		printf("Test %d failed for method %s\n", test, method);
		allPassed = 0;
	}
	test++;

	result = bp[6] == right;
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

	result = GET_RIGHT(root) == right;
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
	SET_SIZE(root, 2*WSIZE);
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

	result = GET_SIZE_T(root) == 8;
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
	result = (GET_RB(root));
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
	SET_RB(root, 1);
	SET_RB(right, 0);
	SET_RB(leaf, 0);

        result = (GET_RB(left));
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
        }
        test++;

	result = (GET_RB(root));
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
        SET_ALLOC(root, 2);
        SET_ALLOC(right, 0);
        SET_ALLOC(leaf, 0);

        result = (GET_ALLOC_T(left));
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
        }
        test++;

	result = (GET_ALLOC_T(root));
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

        SET_PARENT(left, root);
        SET_PARENT(root, 0);
        SET_PARENT(right, root);
        SET_PARENT(leaf, left);

        result = bp[4] == root;
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
	  allPassed = 0;
	  if(verbose) printVerbose(root, bp[4]);
        }
        test++;

        result = bp[10] == 0;
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(0, bp[10]);
        }
        test++;

        result = bp[17] == root;
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(root, bp[17]);
        }
        test++;

        result = bp[22] == left;
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(left, bp[22]);
        }
        test++;




	strcpy(method, "SET_PARENT()");

        result = (GET_PARENT(left)) == root;
        if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
        }
        test++;

	result = (GET_PARENT(root)) == 0;
	if (!result) {
          printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(7, bp[2]);
        }
        test++;

	result = (GET_PARENT(right)) == root;
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

	result = GET_NEXT(left) == root;
	if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(left, result);
        }
        test++;

	result = GET_NEXT(root) == right;
	if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(right, GET_NEXT(root));
        }
        test++;

	result = GET_NEXT(right) == leaf;
	if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(leaf, GET_NEXT(right));
        }
        test++;




        strcpy(method, "GET_LAST()");
	int * r = ((GET_LEFT( GET(MC(root)-4*WSIZE)) == root ) ? (GET_LEFT(GET(MC(root)-4*WSIZE))) : (GET_RIGHT(GET(MC(root)-4*WSIZE))));
	printf("(GET_LEFT( GET(MC(root)-4*WSIZE)) == root: %d\n", (GET_LEFT( GET(MC(root)-4*WSIZE)) == root) );
	printf("4*WSIZE == root: %d\n", (GET_LEFT( GET(MC(root)-4*WSIZE)) == root) );
	result = r == left;
	if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(left, r);
        }
        test++;
/*
	result = GET_LAST(right) == root;
	if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(right, GET_NEXT(root));
        }
        test++;

	result = GET_LAST(leaf) == right;
	if (!result) {
	  printf("Test %d failed for method %s\n", test, method);
          allPassed = 0;
          if(verbose) printVerbose(leaf, GET_NEXT(right));
        }
        test++;
*/


/* Node macros
#define MC(p)			((char*) (p))
#define SET_LEFT(p, addr)	PUT(MC(p)-(3*WSIZE), addr)
#define SET_RIGHT(p, addr)	PUT(MC(p)-(2*WSIZE), addr)
#define SET_SIZE(p, size) 	PUT(MC(p)-WSIZE, PACK_T(size, GET_ALLOC_T(p), GET_RB(p)))
#define SET_PARENT(p, addr)	PUT(MC(p)+GET_SIZE_T(p), addr)
#define SET_ALLOC(p, alloc)	PUT( MC(p)-WSIZE, PACK_T( GET_SIZE_T(p), alloc, GET_RB(p) ) )
#define SET_RB(p, rb)           PUT( MC(p)-WSIZE, PACK_T( GET_SIZE_T(p), GET_ALLOC(p), rb ) )
#define PACK_T(size, alloc, RB) ((size) | (!!((unsigned int) alloc)) | ((!!((unsigned int) RB))<<1))

#define GET_LEFT(p)		GET(MC(p)-3*(WSIZE))
#define GET_RIGHT(p)		GET(MC(p)-2*(WSIZE))
#define GET_ALLOC_T(p)		(GET(MC(p)-1*(WSIZE)) & 0x1)
#define GET_SIZE_T(p)		(GET(MC(p)-1*(WSIZE)) & ~0x3)
#define GET_RB(p)		(GET(MC(p)-1*(WSIZE)) & 0x2)
#define GET_PARENT(p)		GET(MC(p)+GET_SIZE_T(p))
#define GET_LAST(p)		((GET_LEFT( GET(MC(root)-4*WSIZE)) == p ) ? (GET_LEFT(GET(MC(root)-4*WSIZE))) : (GET_RIGHT(GET(MC(root)-4*WSIZE))))
#define GET_NEXT(p)		((MC(p)+4*WSIZE+GET_SIZE_T(p)))
*/





	if(allPassed) {
		printf("All %d tests passed\n\n", test-1);
	}
}
