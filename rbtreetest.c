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

// Read and write a word at address p
#define GET(p)	(*((unsigned int *) (p)))
#define PUT(p, val) (*((unsigned int *) (p)) = ((unsigned int) val))

/* single word (4) or double word (8) alignment */
#define ALIGNMENT 4

//Read the size and allocated fields from address p
#define GET_SIZE(p) 	(GET(p) & ~0x7)
#define GET_ALLOC(p) 	(GET(p) & 0x1)

/* rounds up to the nearest multiple of ALIGNMENT */
#define ALIGN(size) (((size) + (ALIGNMENT-1)) & ~0x3)

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

int* createTree();
int* createTree2();
int testRoot(int* root);
int testOrdering(int* root);
int testRedNodes(int* root);
int testBlackPaths(int* root);

static int num_nodes = 1;
static int * root = NULL;
static int * bp = NULL;

void print_node(int * node);
int insert(int * node);
int* find(int nsize); // find the parent that leaf of size nsize would have
int* rem_delete(int size);
void delete(int *node);
void print_tree_level();
int* get_last(int* bp);
void icase2(int * node);
void print_tree_level(struct lnode* olist);



int main(int argv, char** argc) {
	int * r = createTree2();
	int passed = 1;
	if (argv > 1 && argc[1][0] == 'p') {
		struct lnode* printtree = malloc(sizeof(struct lnode));
		printtree->data = r;
		printtree->next = NULL;
		print_tree_level(printtree);
		printf("\n");
	} else {
		if(!testRoot(r)) { printf("Root was not Black\n"); passed = 0; }
		if(!testOrdering(r)) { printf("Ordering not maintained\n"); passed = 0; }
		if(!testRedNodes(r)) { printf("Red Nodes had red children\n"); passed = 0; }
		if(testBlackPaths(r) == -1) { printf("Tree not balanced\n"); passed = 0; }
		if (passed) printf("All tests passed!\n");
	}
}

int* createTree() {
	int size = 8;
	int* r = malloc(4*WSIZE + size);
	r += 3;
	SET_LEFT(r, NULL);
	SET_RIGHT(r, NULL);
	SET_SIZE(r, size);
	SET_ALLOC(r, 0);
	SET_RB(r, BLACK);
	SET_PARENT(r, NULL);

	size = 4;
	int* lay2_1 = malloc(4*WSIZE + size);
	lay2_1 += 3;
	SET_LEFT(lay2_1, NULL);
	SET_RIGHT(lay2_1, NULL);
	SET_SIZE(lay2_1, size);
	SET_ALLOC(lay2_1, 0);
	SET_RB(lay2_1, BLACK);
	SET_PARENT(lay2_1, r);
	SET_LEFT(r, lay2_1);

	size = 8;
	int* lay3_2 = malloc(4*WSIZE + size);
	lay3_2 += 3;
	SET_LEFT(lay3_2, NULL);
	SET_RIGHT(lay3_2, NULL);
	SET_SIZE(lay3_2, size);
	SET_ALLOC(lay3_2, 0);
	SET_RB(lay3_2, BLACK);
	SET_PARENT(lay3_2, lay2_1);
	SET_RIGHT(lay2_1, lay3_2);

	size = 4;
	int* lay3_1 = malloc(4*WSIZE + size);
	lay3_1 += 3;
	SET_LEFT(lay3_1, NULL);
	SET_RIGHT(lay3_1, NULL);
	SET_SIZE(lay3_1, size);
	SET_ALLOC(lay3_1, 0);
	SET_RB(lay3_1, BLACK);
	SET_PARENT(lay3_1, lay2_1);
	SET_LEFT(lay2_1, lay3_1);

	size = 4;
	int* lay4_1 = malloc(4*WSIZE + size);
	lay4_1 += 3;
	SET_LEFT(lay4_1, NULL);
	SET_RIGHT(lay4_1, NULL);
	SET_SIZE(lay4_1, size);
	SET_ALLOC(lay4_1, 0);
	SET_RB(lay4_1, RED);
	SET_PARENT(lay4_1, lay3_1);
	SET_LEFT(lay3_1, lay4_1);

	size = 12;
	int* lay2_2 = malloc(4*WSIZE + size);
	lay2_2 += 3;
	SET_LEFT(lay2_2, NULL);
	SET_RIGHT(lay2_2, NULL);
	SET_SIZE(lay2_2, size);
	SET_ALLOC(lay2_2, 0);
	SET_RB(lay2_2, BLACK);
	SET_PARENT(lay2_2, r);
	SET_RIGHT(r, lay2_2);

	size = 12;
	int* lay3_3 = malloc(4*WSIZE + size);
	lay3_3 += 3;
	SET_LEFT(lay3_3, NULL);
	SET_RIGHT(lay3_3, NULL);
	SET_SIZE(lay3_3, size);
	SET_ALLOC(lay3_3, 0);
	SET_RB(lay3_3, BLACK);
	SET_PARENT(lay3_3, lay2_2);
	SET_LEFT(lay2_2, lay3_3);

	size = 20;
	int* lay3_4 = malloc(4*WSIZE + size);
	lay3_4 += 3;
	SET_LEFT(lay3_4, NULL);
	SET_RIGHT(lay3_4, NULL);
	SET_SIZE(lay3_4, size);
	SET_ALLOC(lay3_4, 0);
	SET_RB(lay3_4, RED);
	SET_PARENT(lay3_4, lay2_2);
	SET_RIGHT(lay2_2, lay3_4);

	size = 16;
	int* lay4_2 = malloc(4*WSIZE + size);
	lay4_2 += 3;
	SET_LEFT(lay4_2, NULL);
	SET_RIGHT(lay4_2, NULL);
	SET_SIZE(lay4_2, size);
	SET_ALLOC(lay4_2, 0);
	SET_RB(lay4_2, BLACK);
	SET_PARENT(lay4_2, lay3_4);
	SET_LEFT(lay3_4, lay4_2);

	size = 24;
	int* lay4_3 = malloc(4*WSIZE + size);
	lay4_3 += 3;
	SET_LEFT(lay4_3, NULL);
	SET_RIGHT(lay4_3, NULL);
	SET_SIZE(lay4_3, size);
	SET_ALLOC(lay4_3, 0);
	SET_RB(lay4_3, BLACK);
	SET_PARENT(lay4_3, lay3_4);
	SET_RIGHT(lay3_4, lay4_3);

	size = 20;
	int* lay5_1 = malloc(4*WSIZE + size);
	lay5_1 += 3;
	SET_LEFT(lay5_1, NULL);
	SET_RIGHT(lay5_1, NULL);
	SET_SIZE(lay5_1, size);
	SET_ALLOC(lay5_1, 0);
	SET_RB(lay5_1, RED);
	SET_PARENT(lay5_1, lay4_3);
	SET_LEFT(lay4_3, lay5_1);

	size = 24;
	int* lay5_2 = malloc(4*WSIZE + size);
	lay5_2 += 3;
	SET_LEFT(lay5_2, NULL);
	SET_RIGHT(lay5_2, NULL);
	SET_SIZE(lay5_2, size);
	SET_ALLOC(lay5_2, 0);
	SET_RB(lay5_2, RED);
	SET_PARENT(lay5_2, lay4_3);
	SET_RIGHT(lay4_3, lay5_2);	




	return r;
}

int* createTree2() {

}

int testRoot(int* r) {
	if (r == NULL) return 0;
	else return (GET_RB(r) == BLACK);
}

int testOrdering(int* r) {
	if (r == NULL) return 1;
	int out = 1;
	if (GET_LEFT(r) != NULL) out = out && GET_SIZE(r) >= GET_SIZE(GET_LEFT(r));
	if (GET_RIGHT(r) != NULL) out = out && GET_SIZE(r) <= GET_SIZE(GET_LEFT(r));
	out = out && testOrdering(GET_LEFT(r));
	out = out && testOrdering(GET_RIGHT(r));
	return out;
}

int testRedNodes(int* r) {
	if (r == NULL) return 1;
	int out = 1;
	if (GET_RB(r) == RED) {
		if(GET_LEFT(r) != NULL)
			out = out && GET_RB( GET_LEFT(r) ) == BLACK;
		if(GET_RIGHT(r) != NULL)
			out = out && GET_RB( GET_RIGHT(r) ) == BLACK;
	}
}

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
	num_nodes++;

	if(num_nodes == 0) {  //base case
		root = node;
		SET_RB(root, BLACK);
		SET_PARENT(root, NULL);
		return 1;
	}

	int* parent = find(size);
	if(size <= GET_SIZE_T(parent)) {
		SET_LEFT(parent,node);
		SET_PARENT(node, parent);
	} else {
		SET_RIGHT(parent, node);
		SET_PARENT(node, parent);
	}
	

	if(GET_RB(parent) == BLACK) {  // case 1
		return 1;
	}

	if(GET_RB(parent) == RED) { // case 2 and 3
		icase2(node);
	}

	return -1;
}

void icase2(int * node) {
	
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
	int* parent = (int*)GET_PARENT(node);
	if(parent == 0) return NULL;

	int size = GET_SIZE_T(node);
	int psize = GET_SIZE_T(GET_PARENT(node));
	if((int*)GET_LEFT(parent) == node) {
		return (int *) GET_RIGHT(parent);
	}
	return (int*) GET_LEFT(parent);
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

