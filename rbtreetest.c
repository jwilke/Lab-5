#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
#define SET_LEFT(p, addr)	((p != NULL) ? PUT(MC(p)-(3*WSIZE), addr):(unsigned int)p)
#define SET_RIGHT(p, addr)	((p != NULL) ? PUT(MC(p)-(2*WSIZE), addr):(unsigned int)p)
#define SET_SIZE(p, size) 	PUT(MC(p)-WSIZE, PACK_T(size, GET_ALLOC_T(p), GET_RB(p)))
#define SET_PARENT(p, addr)	((p != NULL) ? PUT(MC(p)+GET_SIZE_T(p), addr):(unsigned int)p)
#define SET_ALLOC(p, alloc)	PUT( MC(p)-WSIZE, PACK_T( GET_SIZE_T(p), alloc, GET_RB(p) ) )
#define SET_RB(p, rb)           PUT( MC(p)-WSIZE, PACK_T( GET_SIZE_T(p), GET_ALLOC(p), rb ) )
#define PACK_T(size, alloc, RB) ((size) | (!!((unsigned int) alloc)) | ((!!((unsigned int) RB))<<1))

#define GET_LEFT(p)		((p != NULL) ? ( (int *) GET(MC(p)-3*(WSIZE)) ):NULL)
#define GET_RIGHT(p)		((p != NULL) ? ( (int *) GET(MC(p)-2*(WSIZE)) ):NULL)
#define GET_ALLOC_T(p)		(GET(MC(p)-1*(WSIZE)) & 0x1)
#define GET_SIZE_T(p)		(GET(MC(p)-1*(WSIZE)) & ~0x3)
#define GET_RB(p)		((p != NULL) ? ((GET(MC(p)-1*(WSIZE)) & 0x2) >> 1):0)
#define GET_PARENT(p)		((p != NULL) ? ( (int *) GET(MC(p)+GET_SIZE_T(p)) ):NULL)
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

static int num_nodes = 0;
static int * root = NULL;
static int * basepointer = NULL;

int* get_last(int* bp);
void print_node(int * node);
int insert(int * node);
int* rem_find(int nsize);
int* find(int nsize); // find the parent that leaf of size nsize would have
int* rem_delete(int size);
int* delete(int *node);
void printTree();
void print_tree_level(struct lnode* olist);
int* get_sibling(int* node);
void icase1(int * node);
void icase2(int * node);
void icase3(int * node);
void icase4(int * node);
void icase5(int * node);
void rotate_clock(int * node);
void rotate_counter_clock(int * node);
int * createNode(int * base, int size);
void printBlock(int size);
void printBlockNodes(int size);
int runTests(int* r, int size, int round);
int* getSucPre(int* node);
void dcase1(int * n);
void dcase2(int * n);
void dcase3(int * n);
void dcase4(int * n);
void dcase5(int * n);
void dcase6(int * n);
int * delete_sub(int * n);
void swap_nodes(int* a, int* b);

int main(int argv, char** argc) {
	int * r = createTree2();
	int passed = 1;
	if (argv > 1 && argc[1][0] == 'p') {
	        printTree();
		printf("\n");
	} else {
	  printf("*************************\nFinal\n");
		if(!testRoot(r)) { printf("Root was not Black\n"); passed = 0; }
		if(!testOrdering(r)) { printf("Ordering not maintained\n"); passed = 0; }
		if(!testRedNodes(r)) { printf("Red Nodes had red children\n"); passed = 0; }
		if(testBlackPaths(r) == -1) { printf("Tree not balanced\n"); passed = 0; }
		if (passed) { 
		  printf("All tests passed!\n");
		} else {
		  printBlock(3*4 + 6);
		  printTree();
		}
	}
}

int runTests(int* r, int size, int round) {
  printf("**************************\n");
  printf("Round: %d\n", round);
  int passed = 1;
  if(!testRoot(r)) { printf("Root was not Black\n"); passed = 0; }
  if(!testOrdering(r)) { printf("Ordering not maintained\n"); passed = 0; }
  if(!testRedNodes(r)) { printf("Red Nodes had red children\n"); passed = 0; }
  if(testBlackPaths(r) == -1) { printf("Tree not balanced\n"); passed = 0; }
  if (passed) {
    printf("All tests passed!\n");
  } else {
    printBlock(3*4 + 6);
    printTree();
  }
}

int* createTree() {
	int size = 8;
	int round = 1;
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
  int size = 16*6 + 4 + 8 + 12 + 16 + 20 + 24;
  int * space = malloc(size);
  int round = 1;
  basepointer = space;
  int * node = space + 3;
  
  createNode(node, 24);
  insert(node);
  node = GET_NEXT(node);

  runTests(root, 5, round++);

  createNode(node, 8);
  insert(node);
  node = GET_NEXT(node);

  runTests(root, 5, round++);

  createNode(node, 16);
  insert(node);
  node = GET_NEXT(node);

  runTests(root, 5, round++);

  createNode(node, 4);
  insert(node);
  node = GET_NEXT(node);

  runTests(root, 5, round++);


  /*
  createNode(node, 20);
  insert(node);
  node = GET_NEXT(node);

  runTests(root, 5, round++);

  createNode(node, 24);
  insert(node);

  runTests(root, 5, round++);

  node = rem_delete(24);
  runTests(root, 5, round++);

  insert(node);
  runTests(root, 5, round++);

  node = rem_delete(20);
  runTests(root, 5, round++);

  insert(node);
  runTests(root, 5, round++);
  */
  printTree();
  node = rem_delete(8);
printTree();
  runTests(root, 5, round++);
  printTree();
  printBlock(4*4 + 1+2+4+6);
  return root;
}

int * createNode(int * base, int size) {
  SET_LEFT(base, NULL);
  SET_RIGHT(base, NULL);
  SET_SIZE(base, size);
  SET_ALLOC(base, 0);
  SET_RB(base, BLACK);
  SET_PARENT(base, NULL);
}

int testRoot(int* r) {
	if (r == NULL) return 0;
	else return (GET_RB(r) == BLACK);
}

int testOrdering(int* r) {
  if (r == NULL) return 1;
	int out = 1;
	if (GET_LEFT(r) != NULL) out = out && GET_SIZE(r) >= GET_SIZE(GET_LEFT(r));
	if (GET_RIGHT(r) != NULL) out = out && GET_SIZE(r) <= GET_SIZE(GET_RIGHT(r));
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

int insert(int * node) { //assumes header/footer is already created
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
  if (root == node) root = r;
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
  if (root == node) root = l;
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

int* rem_find(int nsize) {
  int* current = root;

  while(1) {
    if(nsize == GET_SIZE_T(current)) break;
    if(nsize < GET_SIZE_T(current)) { //try to go left
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

int* rem_delete(int size) {
  // find node to delete
  int * rem = rem_find(size);
printf("size of node: %d\n\n\n", GET_SIZE_T(rem));
  // delete node
  delete(rem);
  // return node
  return rem;
}

int* delete(int *node) {
	num_nodes--;
	int* p = GET_PARENT(node);
	int* l = GET_LEFT(node);
	int* r = GET_RIGHT(node);
	int* sorp = getSucPre(node);
	if(sorp == NULL) {
		printf("size of node: %d\n\n\n", GET_SIZE_T(node));
	}
	swap_nodes(node, sorp);
	int* n = delete_sub(node);

	/*if(n == NULL) {
		if(p != NULL) {
			if( node == GET_LEFT(p) ) {
				SET_LEFT(p, NULL);
			} else {
				SET_RIGHT(p, NULL);
			}
		} else {
			root = NULL;
		}
		return node;
	}

	if(p != NULL) {
		if( node == GET_LEFT(p) ) {
			SET_LEFT(p, n);
		} else {
			SET_RIGHT(p, n);
		}
	} else {
		root = n;
	}
	
	SET_LEFT(n, l);
	SET_PARENT(l, n);
	SET_RIGHT(n, r);
	SET_PARENT(r, n);
	SET_PARENT(n, p);*/

	return node;
}




int* delete_sub(int* n) {
	if(n == NULL) return NULL;
	int* child;
	int* copy = n;
	if (GET_LEFT(n) == NULL) {
		child = GET_RIGHT(n);
	} else {
		child = GET_LEFT(n);
	}

	int* p = GET_PARENT(n);
	if(child != NULL) {
		if(p != NULL) {
			SET_PARENT(child, p);
			if(GET_LEFT(p) == n) {
				SET_LEFT(p, child);
			} else {
				SET_RIGHT(p, child);
			}
		} else {
			root = child;
		}
	} else {
		if (p != NULL) {
			if(GET_LEFT(p) == n) {
				SET_LEFT(p, NULL);
			} else {
				SET_RIGHT(p, NULL);
			}
		} else {
			root = NULL;
		}
	}


	if(GET_RB(n) == BLACK) {
		if (GET_RB(child) == RED) {
			SET_RB(child, BLACK);
		} else {
			dcase1(child);
		}
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



int* getSucPre(int* node) {
  int* temp;
  if ((temp = GET_LEFT(node)) != NULL) {
    while(GET_RIGHT(node) != NULL) {
      temp = GET_RIGHT(node);
    }
    return temp;
  } else if ((temp = GET_RIGHT(node)) != NULL) {
    while(GET_LEFT(node) != NULL) {
      temp = GET_LEFT(node);
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















void printTree() {
  struct lnode* printtree = malloc(sizeof(struct lnode));
  printtree->data = root;
  printtree->next = NULL;
  print_tree_level(printtree);
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

void printBlock(int size) {
  int i;
  printf("**************************\n");
  for(i = 0; i < size; i++ ) {
    printf("%p: %x\n", basepointer + i, basepointer[i]);
  }
  printf("**************************\n");
}

void printBlockNodes(int size) {
  int * current = basepointer + 3;
  while (current < basepointer + size) {
    print_node(current);
    printf("\n");
    current = GET_NEXT(current);
  }

}

void swap_nodes(int* a, int* b) {
	int ca = GET_RB(a);
	int* la = GET_LEFT(a);
	int* ra = GET_RIGHT(a);
	int* pa = GET_PARENT(a);
	
	int cb = GET_RB(b);
	int* lb = GET_LEFT(b);
	int* rb = GET_RIGHT(b);
	int* pb = GET_PARENT(b);

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

	if(b == GET_LEFT(pb)) {
		SET_LEFT(pb, a);
	} else {
		SET_RIGHT(pb, a);
	}

	if(a == GET_LEFT(pa)) {
		SET_LEFT(pa, b);
	} else {
		SET_RIGHT(pa, b);
	}
	if(root == a) root = b;
	else if(root == b) root = a;
}



/*		
	// if it is a red child nothing needs to be done
	if (GET_LEFT(node) == NULL && GET_RIGHT(node) == NULL && GET_RB(node) == RED) {
		int * parent = GET_PARENT(node);
	if (GET_LEFT(parent) == node) {
		SET_LEFT(parent, NULL);
	} else {
		SET_RIGHT(parent, NULL);
	}
		SET_PARENT(node, NULL);
		return node;
	}

  // node is black with one red child
  if(GET_RB(node) == BLACK) {
    // it has a single red child (has to be a leaf) to the right
    if( (GET_LEFT(node) == NULL && GET_RIGHT(node) != NULL && GET_RB(GET_RIGHT(node)) == RED) ) {
      // switch with right
      int* rep = GET_RIGHT(node);
      int* parent = GET_PARENT(node);
      if(parent!= NULL) {
        if (GET_LEFT(parent) == node) {
          SET_LEFT(parent, rep);
        } else {
          SET_RIGHT(parent, rep);
        }
      } else {
	// must be root
        assert(root == node);
        root = rep;
      }
      SET_RB(rep, BLACK);
      return node;
    }

    // it has a single red child (has to be a leaf) to the left
    if( (GET_RIGHT(node) == NULL && GET_LEFT(node) != NULL && GET_RB(GET_LEFT(node)) == RED) ) {
      //switch with left
      int* rep = GET_LEFT(node);
      int* parent = GET_PARENT(node);
      if(parent != NULL) {
	if (GET_LEFT(parent) == node) {
	  SET_LEFT(parent, rep);
	} else {
	  SET_RIGHT(parent, rep);
	} 
      } else {
	// must be root
	assert(root == node);
        root = rep;
      }
      SET_RB(rep, BLACK);
      return node;
    }

  }

  // black leaf - more difficult
  if (GET_LEFT(node) == NULL && GET_RIGHT(node) == NULL && GET_RB(node) == BLACK) {
    int blackleaf = 0;
    assert(blackleaf);
  }

  // get and replace node
  if (GET_LEFT(node) != NULL && GET_RIGHT(node) != NULL) {
    int* exchange = getSucPre(node);
    delete(exchange);
    SET_LEFT(exchange, GET_LEFT(node));
    SET_RIGHT(exchange, GET_RIGHT(node));
    SET_PARENT(exchange, GET_PARENT(node));
    SET_RB(exchange, GET_RB(node));
    int* parent = GET_PARENT(node);
    if(parent != NULL) {
      if (GET_LEFT(parent) == node) {
	SET_LEFT(parent, exchange);
      } else {
	SET_RIGHT(parent, exchange);
      }
    } else {
      root = exchange;
      SET_RB(root, BLACK);
    }
    num_nodes++;
    return node;
  }
	*/
