/*
TODO:
complete the rest of the insertion cases, just finished case 3;
start and finish rem_delete and delete;
*/

#include <stdio.h>
#include <stdlib.h>

struct lnode {
	int *data;
	struct lnode * next;
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

#define GET_LEFT(p)		( (int *) GET(MC(p)-3*(WSIZE)) )
#define GET_RIGHT(p)		( (int *) GET(MC(p)-2*(WSIZE)) )
#define GET_ALLOC_T(p)		(GET(MC(p)-1*(WSIZE)) & 0x1)
#define GET_SIZE_T(p)		(GET(MC(p)-1*(WSIZE)) & ~0x3)
#define GET_RB(p)		((GET(MC(p)-1*(WSIZE)) & 0x2) >> 1)
#define GET_PARENT(p)		( (int *) GET(MC(p)+GET_SIZE_T(p)) )
#define GET_LAST(p)		get_last(p)
#define GET_NEXT(p)		( (int *) ((MC(p)+4*WSIZE+GET_SIZE_T(p))) )

#define OVERHEAD 16

#define BLACK 0
#define RED   1

static int num_nodes = 1;
static int * root = NULL;
static int * bp = NULL;

void print_node(int * node);
int insert(int * node);
int* find(int nsize); // find the parent that leaf of size nsize would have
int* rem_delete(int size);
void delete(int *node);
void print_tree_level(struct lnode* olist);
int* get_sibling(int* node);
void icase1(int * node);
void icase2(int * node);
void icase3(int * node);
void icase4(int * node);
void icase5(int * node);
void rotate_clock(int * node);
void rotate_counter_clock(int * node);

#define MAX_BYTES 512

int main() {
	bp = malloc(MAX_BYTES);
	root = bp+3;
	
	SET_LEFT(root, NULL);
	SET_RIGHT(root, NULL);
	SET_SIZE(root, 8);
	SET_ALLOC(root, 0);
	SET_RB(root, BLACK);
	SET_PARENT(root, NULL);

	int * temp = (int*)GET_NEXT(root);
	
	SET_LEFT(temp, NULL);
	SET_RIGHT(temp, NULL);
	SET_SIZE(temp, 16);
	SET_ALLOC(temp, 0);
	SET_RB(temp, BLACK);
	SET_PARENT(temp, NULL);

	insert(temp);

	temp = (int*)GET_NEXT(temp);

	SET_LEFT(temp, NULL);
	SET_RIGHT(temp, NULL);
	SET_SIZE(temp, 4);
	SET_ALLOC(temp, 0);
	SET_RB(temp, BLACK);
	SET_PARENT(temp, NULL);

	insert(temp);

	struct lnode* printtree = malloc(sizeof(struct lnode));
	printtree->data = root;
	printtree->next = NULL;
	print_tree_level(printtree);


	free(bp);
	return 0;
}

void print_node(int * node) {
	printf("\nNODE:\nLEFT: %p\n", GET_LEFT(node));
	printf("RIGHT: %p\n", GET_RIGHT(node));
	printf("SIZE: %d\n", GET_SIZE_T(node));
	printf("RB: %d\n", GET_RB(node));
	printf("PARENT: %p\n", GET_PARENT(node));
	return;
}

int insert(int * node) {  //assumes header/footer is already created
	int size = GET_SIZE_T(node);

	
	SET_LEFT(node, NULL);
	SET_RIGHT(node, NULL);
	SET_SIZE(node, size);
	SET_RB(node, RED);
	SET_ALLOC(node, 0);

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
		SET_RB(parent, BLACK);		// set parent to black
		SET_RB(u, BLACK);		// set uncle to black
		SET_RB(GET_PARENT(parent),RED); // set grandparent to red
		icase1(GET_PARENT(parent));     // check if grandparent is good
	} else {
		icase4(node);
	}
}

void icase4(int * node) {
	//int p = GET_PARENT(node);
	//int gp =  GET_PARENT(p);
	
	
}

void icase5(int * node) {	
}

void rotate_clock(int * node) {
	int* p = GET_PARENT(node);
	int* g = GET_PARENT(node);
	int go_left = (GET_RIGHT(g) == p);
	int* r = GET_RIGHT(node);

	if(go_left) SET_LEFT(g, node);
	else SET_RIGHT(g, node);

	SET_LEFT(p,r);
	SET_RIGHT(node, p);
}

void rotate_counter_clock(int * node) {
	int* p = GET_PARENT(node);
	int* g = GET_PARENT(node);
	int go_left = (GET_RIGHT(g) == p);
	int* l = GET_LEFT(node);

	if(go_left) SET_LEFT(g, node);
	else SET_RIGHT(g, node);

	SET_RIGHT(p,l);
	SET_LEFT(node, p);
}


int* find(int nsize) {
	int* current = root;

	while(1) {
		if(nsize <= GET_SIZE_T(current)) {  //try to go left
			if(GET_LEFT(current) == 0) { // if there's no node to the left, it's the parent
				break;
			} else {
				current = (int*)GET_LEFT(current); // go left and continue
				continue;
			}
		} else {  // try to go right
			if(GET_RIGHT(current) == 0) { // if there's no node to the right, it's the parent
				break;
			} else {
				current = (int*)GET_RIGHT(current); // go left and continue
			}
		}
		
	}
	return current;
}

int* rem_delete(int size) {
	return NULL;
}

void delete(int *node) {
	return;
}

int* get_sibling(int* node) {
	int* parent = GET_PARENT(node);
	if(parent == 0) return NULL;

	if((int*)GET_LEFT(parent) == node) {
		return GET_RIGHT(parent);
	}
	return GET_LEFT(parent);
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

		printf("%d:%c ", GET_SIZE_T(olist->data), color);
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
































