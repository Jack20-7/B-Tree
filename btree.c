#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define DEGREE      3            //M / 2

typedef int KEY_VALUE;

typedef struct _btree_node{
    KEY_VALUE* keys;
    struct _btree_node** childrens;
    int num;                  //key的个数
    int leaf;                 //该节点是否是叶子节点
}btree_node;

typedef struct _btree{
    btree_node* root;
    int t;  //degree
}btree;

btree_node* btree_create_node(int t,int leaf){
    btree_node* node = (btree_node*)calloc(1,sizeof(btree_node));
    if(node == NULL){
        assert(0);
    }

    node->leaf = leaf;
    node->num = 0;
    node->keys = (KEY_VALUE*)calloc(1,(2 * t - 1) * sizeof(KEY_VALUE));
    node->childrens = (btree_node**)calloc(1,(2 * t) * sizeof(btree_node));

    return node;
}

void btree_destroy_node(btree_node* node){
    assert(node);

    free(node->childrens);
    free(node->keys);
    free(node);
}

void btree_create(btree* T,int t){
    T->t = t;

    btree_node* x = btree_create_node(t,1);
    T->root = x;
}

//分裂
//x代表满了的节点的父节点
//i表示满了的节点在父节点childrens中的下标
void btree_split_child(btree* T,btree_node* x,int i){
    int t = T->t;

    btree_node* y = x->childrens[i];
    btree_node* z = btree_create_node(t,y->leaf);

    z->num = t - 1;
    //将y的后面 t - 1个key弄到z的前面去
    int j = 0;
    for(;j < t - 1;++j){
        z->keys[j] = y->keys[j + t];
    }
    //如果不是叶子节点的话，指针也需要弄进去
    if(y->leaf == 0){
        for(j = 0;j < t;++j){
            z->childrens[j] = y->childrens[j + t];
        }
    }

    //对于y后面t - 1的key，不需要删除了，这里直接将num设置为t - 1，这样后面在插入时，直接覆盖掉就行了
    y->num = t - 1;

    //在x中为z寻找合适的位置
    for(j = x->num;j >= i + 1;--j){
        x->childrens[j + 1] = x->childrens[j];
    }
    x->childrens[j + 1] = z;

    //y中的中间key需要放入到x的合适位置去
    for(j = x->num - 1;j >= i;--j){
        x->keys[j + 1] = x->keys[j];
    }
    x->keys[i] = y->keys[t - 1];
    x->num ++;
}

//将k插入到非空的节点中
void btree_insert_nonfull(btree* T,btree_node* x,KEY_VALUE k){
    //B树所有的节点都是插入到叶子节点中的
    int i = x->num - 1;
    if(x->leaf == 1){
        //如果当前节点是叶子节点的话，那么就可以将k插入合适的位置
        while(i >= 0 && x->keys[i] > k){
            x->keys[i + 1] = x->keys[i];
            i--;
        }
        x->keys[i + 1] = k;
        x->num++;
    }else{
        //如果不是叶子节点的话，那么就需要通过key的值来判断该插入到哪一个子树上
        while(i >= 0 && x->keys[i] > k){
            i--;
        }
        //判断该子节点是否已满
        if(x->childrens[i + 1]->num == (2 * T->t - 1)){
            btree_split_child(T,x,i + 1);
            if(k > x->keys[i + 1]){
                i++;
            }
        }
        btree_insert_nonfull(T,x->childrens[i + 1],k);
    }
}

//下面是B树插入节点的操作
//插入节点涉及到分裂,在插入之前需要先判断要插入的节点是否已满，如果已满的话，就先分裂，在插入
//所有的key都是插入在叶子节点上面的
void btree_insert(btree* T,KEY_VALUE key){
    btree_node* r = T->root;
    if(r->num ==  2 * T->t - 1){
        //如果根节点已满的话，需要先分裂
        btree_node* node = btree_create_node(T->t,0);
        T->root = node;
        node->childrens[0] = r;

        //分裂
        btree_split_child(T,node,0);

        int i = 0;
        if(key > node->keys[0]){
            i++;
        }
        btree_insert_nonfull(T,node->childrens[i],key);
    }else{
        //如果没有满
        btree_insert_nonfull(T,r,key);
    }
}

//遍历B树
void btree_traverse(btree_node* x){
    int i = 0;
    for(;i < x->num;++i){
        if(x->leaf == 0){
            btree_traverse(x->childrens[i]);
        }
        printf("%C ",x->keys[i]);
    }
    if(x->leaf == 0){
        btree_traverse(x->childrens[i]);
    }
}

void btree_print(btree* T,btree_node* node,int layer){
    btree_node* p = node;
    int i = 0;
    if(p){
        printf("\nlayer = %d,keynum = %d,is_leaf = %d\n",layer,p->num,p->leaf);
        for(;i < node->num;++i){
            printf("%c ",p->keys[i]);
        }
        printf("\n");

        layer++;

        for(i = 0;i <= p->num;++i){
            if(p->childrens[i]){
                btree_print(T,p->childrens[i],layer);
            }
        }
    }else{
        printf("the tree is empty\n");
    }
}

int btree_bin_search(btree_node* node,int low,int high,KEY_VALUE key){
    int mid = 0;
    if(low > high || low < 0 || high < 0){
        return -1;
    }

    while(low <= high){
        mid = low + (high - low) / 2;
        if(key > node->keys[mid]){
            low = mid + 1;
        }else{
            high = mid - 1;
        }
    }
    return low;
}

//下面就是B树的删除
/*
 *  B树删除涉及到借位和合并
 *  1.如果删除key不在叶子节点上的话
 *     (1).如果前一个节点keys > t - 1,向前借位
 *     (2).如果后一个节点keys > t - 1,向后借位
 *     (3).合并三个三个节点的keys
 *  2.如果在叶子节点上面的话
 *    直接覆盖掉要删除的值
 *   
 */

void btree_merge(btree* T,btree_node* node,int idx){
    btree_node* left = node->childrens[idx];
    btree_node* right = node->childrens[idx + 1];
    int i = 0;

    left->keys[T->t - 1] = node->keys[idx];
    for(i = 0;i < T->t - 1;++i){
        left->keys[T->t + i] = right->keys[i];
    }
    if(!left->leaf){
        for(i = 0;i < T->t;++i){
            left->childrens[T->t + i] = right->childrens[i];
        }
    }
    left->num += T->t;

    btree_destroy_node(right);

    //该调整node节点的值
    for(i = idx + 1;i < node->num;++i){
        node->keys[i - 1] = node->keys[i];
        node->childrens[i] = node->childrens[i + 1];
    }
    node->childrens[i + 1] = NULL;
    node->num --;
    if(node->num == 0){
        T->root = left;
        btree_destroy_node(node);
    }
}
void btree_delete_key(btree* T,btree_node* node,KEY_VALUE key){
    if(node == NULL){
        return ;
    }
    int idx = 0;
    int i = 0;

    //在该节点上寻找要删除的节点
    while(idx < node->num && key > node->keys[idx]){
        idx++;
    }

    if(idx < node->num && key == node->keys[idx]){
        //如果要删除的key在该节点上面的话

        //判断是否为叶子节点
        if(node->leaf){
            //如果是叶子节点的话，直接覆盖掉
            for(i = idx;i < node->num - 1;i++){
                node->keys[i] = node->keys[i + 1];
            }
            node->keys[node->num - 1] = 0;
            node->num--;

            if(node->num == 0){
                //只有root节点的key才可能 = 1
                free(node);
                T->root = NULL;
            }
            return ;
        }else if(node->childrens[idx]->num >= T->t){
            //向前借位
            btree_node* left = node->childrens[idx];
            node->keys[idx] = left->keys[left->num - 1];
            btree_delete_key(T,left,left->keys[left->num - 1]);
        }else if(node->childrens[idx + 1]->num >= T->t){
            //向后借位
            btree_node* right = node->childrens[idx + 1];
            node->keys[idx] = right->keys[0];
            btree_delete_key(T,right,right->keys[0]);
        }else{
            //合并节点
            btree_merge(T,node,idx);
            btree_delete_key(T,node->childrens[idx],key);
        }
    }else{
        //如果不在该节点上面
        btree_node* child = node->childrens[idx];
        if(child == NULL){
            printf("Cannot del key = %d\n",key);
            return ;
        }
        if(child->num == T->t - 1){
            //如果该节点的key个数 == T->t - 1,就需要借位
            btree_node* left = NULL;
            btree_node* right = NULL;
            if(idx - 1>= 0){
                left = node->childrens[idx - 1];
            }
            if(idx + 1 <= node->num){
                right = node->childrens[idx + 1];
            }
            if((left && left->num >= T->t) ||
                    (right && right->num >= T->t)){
                int richR = 0;
                if(right){
                    richR = 1;
                }
                if(left && right){
                    richR = (right->num > left->num) ? 1 : 0;
                }
                if(right && right->num >= T->t && richR){
                    //从right进行借位
                    child->keys[child->num] = node->keys[idx];
                    child->childrens[child->num + 1] = right->childrens[0];
                    child->num++;

                    node->keys[idx] = right->keys[0];
                    for(i = 0;i < right->num - 1;++i){
                        right->keys[i] = right->keys[i + 1];
                        right->childrens[i] = right->childrens[i + 1];
                    }
                    right->keys[right->num - 1] = 0;
                    right->childrens[right->num - 1] = right->childrens[right->num];
                    right->childrens[right->num] = NULL;
                    right->num--;
                }else{
                    //从left进行借位
                    for(i = child->num;i > 0;--i){
                        child->keys[i] = child->keys[i - 1];             
                        child->childrens[i + 1] = child->childrens[i];
                    }
                    child->childrens[1] = child->childrens[0];
                    child->childrens[0] = left->childrens[left->num];
                    child->keys[0] = node->keys[idx - 1];
                    child->num++;

                    node->keys[idx - 1] = left->keys[left->num - 1];
                    left->keys[left->num - 1] = 0;
                    left->childrens[left->num] = NULL;
                    left->num--;
                }
            }else if((!left || (left->num == T->t - 1))
                        && (!right || (right->num == T->t - 1))){
                if(left && left->num == T->t - 1){
                    btree_merge(T,node,idx - 1);
                    child = left;
                }else if(right && right->num == T->t - 1){
                    btree_merge(T,node,idx);
                }
            }
        }
        btree_delete_key(T,child,key);
    }
}

int btree_delete(btree* T,KEY_VALUE key){
    if(!T->root){
        return -1;
    }

    btree_delete_key(T,T->root,key);
    return 0;
}

int main(int argc,char ** argv){
    btree T = {0};
    btree_create(&T,3);

    int i = 0;
    char key[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    for(i = 0;i < 26;++i){
        printf("%c ",key[i]);
        btree_insert(&T,key[i]);
    }
    btree_print(&T,T.root,0);

    for(i = 0;i < 26;++i){
        printf("\n-----------------------\n");
        btree_delete(&T,key[25 - i]);
        printf("delete %c \n",key[25 - i]);
        btree_print(&T,T.root,0);
    }
    return 0;
}


