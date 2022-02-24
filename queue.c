#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "harness.h"
#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */

/*
 * Create empty queue.
 * Return NULL if could not allocate space.
 */
struct list_head *q_new()
{
    struct list_head *head = malloc(sizeof(struct list_head));
    head->next = head;
    head->prev = head;
    if (head) {
        return head;
    } else {
        return NULL;
    }
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (l) {
        struct list_head *node;
        for (node = l->next; node != l;) {
            element_t *kn = container_of(node, element_t, list);
            node = node->next;
            free(kn->value);
            free(kn);
        }
        free(l);
    }
}

/*
 * Attempt to insert element at head of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_head(struct list_head *head, char *s)
{
    if (!head || !s) {
        return false;
    }
    element_t *node = malloc(sizeof(element_t));
    if (!node) {
        return false;
    }
    node->value = strdup(s);
    if (!node->value) {
        free(node);
        return false;
    }
    struct list_head *next = head->next;
    struct list_head *node_list = &node->list;
    next->prev = node_list;
    node_list->next = next;
    node_list->prev = head;
    head->next = node_list;
    return true;

}

/*
 * Attempt to insert element at tail of queue.
 * Return true if successful.
 * Return false if q is NULL or could not allocate space.
 * Argument s points to the string to be stored.
 * The function must explicitly allocate space and copy the string into it.
 */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (!head || !s) {
        return false;
    }
    element_t *node = malloc(sizeof(element_t));
    if (!node) {
        return false;
    }
    node->value = strdup(s);
    if (!node->value) {
        free(node);
        return false;
    }
    struct list_head *prev = head->prev;
    struct list_head *node_list = &node->list;
    prev->next = node_list;
    node_list->next = head;
    node_list->prev = prev;
    head->prev = node_list;
    return true;
}

/*
 * Attempt to remove element from head of queue.
 * Return target element.
 * Return NULL if queue is NULL or empty.
 * If sp is non-NULL and an element is removed, copy the removed string to *sp
 * (up to a maximum of bufsize-1 characters, plus a null terminator.)
 *
 * NOTE: "remove" is different from "delete"
 * The space used by the list element and the string should not be freed.
 * The only thing "remove" need to do is unlink it.
 *
 * REF:
 * https://english.stackexchange.com/questions/52508/difference-between-delete-and-remove
 */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || head->next == head) {
        return NULL;
    } else {
        element_t *kh = container_of(head->next, element_t, list);
        memset(sp, '\0', bufsize);
        strncpy(sp, kh->value, bufsize);
        list_del_init(&(kh->list));
        return kh;
    }
}

/*
 * Attempt to remove element from tail of queue.
 * Other attribute is as same as q_remove_head.
 */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || head->prev == head) {
        return NULL;
    } else {
        element_t *kh = container_of(head->prev, element_t, list);
        memset(sp, '\0', bufsize);
        strncpy(sp, kh->value, bufsize);
        list_del_init(&(kh->list));
        return kh;
    }
}

/*
 * WARN: This is for external usage, don't modify it
 * Attempt to release element.
 */
void q_release_element(element_t *e)
{
    free(e->value);
    free(e);
}

/*
 * Return number of elements in queue.
 * Return 0 if q is NULL or empty
 */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;
    int len = 0;
    struct list_head *li;
    list_for_each (li, head)
        len++;
    return len;
}

/*
 * Delete the middle node in list.
 * The middle node of a linked list of size n is the
 * ⌊n / 2⌋th node from the start using 0-based indexing.
 * If there're six element, the third member should be return.
 * Return true if successful.
 * Return false if list is NULL or empty.
 */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (head && head->next != head) {
        struct list_head *fast = head->next;
        struct list_head *slow = head->next;
        while (fast != head && fast->next != head) {
            fast = fast->next->next;
            slow = slow->next;
        }
        element_t *kn = container_of(slow, element_t, list);
        list_del_init(slow);
        free(kn->value);
        free(kn);
        return true;
    } else {
        return false;
    }
}

/*
 * Delete all nodes that have duplicate string,
 * leaving only distinct strings from the original list.
 * Return true if successful.
 * Return false if list is NULL.
 *
 * Note: this function always be called after sorting, in other words,
 * list is guaranteed to be sorted in ascending order.
 */
bool dup(struct list_head *head, struct list_head *stay)
{
    if (head->next == stay) {
        return false;
    } else {
        element_t *kn = container_of(head, element_t, list);
        element_t *kn_next = container_of(head->next, element_t, list);
        if (!strcmp(kn->value, kn_next->value)) {
            return true;
        }
        return false;
    }
}
struct list_head *delete_dup(struct list_head *head, struct list_head *stay)
{
    if (head == stay) {
        return stay;
    }
    if (dup(head, stay)) {
        while (dup(head, stay)) {
            element_t *kn = container_of(head, element_t, list);
            struct list_head *prev = head->prev;
            head->prev = NULL;
            head = head->next;
            head->prev = prev;
            free(kn->value);
            free(kn);
        }
        struct list_head *temp = delete_dup(head->next, stay);
        temp->prev = head->prev;
        element_t *kn = container_of(head, element_t, list);
        free(kn->value);
        free(kn);
        return temp;
    }
    struct list_head *next = delete_dup(head->next, stay);
    head->next = next;
    next->prev = head;
    return head;
}
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head) {
        return false;
    }
    head->next = delete_dup(head->next, head);
    return true;
}

/*
 * Attempt to swap every two adjacent nodes.
 */
struct list_head *swap(struct list_head *one, struct list_head *two)
{
    struct list_head *temp = two->next;
    two->next = one;
    two->prev = one->prev;
    one->prev = two;
    temp->prev = one;
    one->next = temp;
    return two;
}
struct list_head *r_swap(struct list_head *head, struct list_head *stay)
{
    if (head == stay || head->next == stay) {
        return head;
    } else {
        struct list_head *sp_list_head = r_swap(head->next->next, stay);
        head->next->next = sp_list_head;
        sp_list_head->prev = head;
        head = swap(head, head->next);
        return head;
    }

}
void q_swap(struct list_head *head)
{
    if (head) {
        struct list_head *stay = head;
        head->next = r_swap(head->next, stay);
    }
    // https://leetcode.com/problems/swap-nodes-in-pairs/
}

/*
 * Reverse elements in queue
 * No effect if q is NULL or empty
 * This function should not allocate or free any list elements
 * (e.g., by calling q_insert_head, q_insert_tail, or q_remove_head).
 * It should rearrange the existing ones.
 */
void q_reverse(struct list_head *head)
{
    if (!head || head->next == head) {
        return;
    }
    struct list_head *stay = head;
    head = head->next;
    struct list_head *next = head->next;
    while (head != stay) {
        head->next = head->prev;
        head->prev = next;
        head = next;
        next = next->next;
    }
    head->next = head->prev;
    head->prev = next;
}

/*
 * Sort elements of queue in ascending order
 * No effect if q is NULL or empty. In addition, if q has only one
 * element, do nothing.
 */
void half(struct list_head *head,
          struct list_head **left,
          struct list_head **right)
{
    struct list_head *fast = head->next;
    struct list_head *slow = head;
    while (fast != head && fast->next != head) {
        fast = fast->next->next;
        slow = slow->next;
    }
    printf("finished\r\n");
    struct list_head *right_node = slow->next;
    struct list_head *rlast = head->prev;
    element_t *l_element = container_of(rlast, element_t, list);
    printf("head->prev=%s\r\n", l_element->value);
    (*right)->next = right_node;
    printf("right next asigned\r\n");
    (*right)->prev = rlast;
    printf("right finished\r\n");
    right_node->prev = *right;
    slow->next = head;
    printf("slow next\r\n");
    rlast->next = *right;
    head->prev = slow;
    *left = head;
}
struct list_head *merge(struct list_head *left, struct list_head *right)
{
    struct list_head *head = q_new();
    struct list_head *left_stay = left;
    struct list_head *right_stay = right;
    left = left->next;
    right = right->next;
    while (left_stay != left && right_stay != right) {
        element_t *l_element = container_of(left, element_t, list);
        element_t *r_element = container_of(right, element_t, list);
        struct list_head *nprev;
        if (l_element->value < r_element->value) {
            nprev = left;
            left = left->next;
            (nprev->prev)->next = left;
            (nprev->next)->prev = nprev->prev;
            nprev->next = head->next;
            nprev->prev = head;
            (head->next)->prev = nprev;
            head->next = nprev;
        } else {
            nprev = right;
            left = right->next;
            (nprev->prev)->next = right;
            (nprev->next)->prev = nprev->prev;
            nprev->next = head->next;
            nprev->prev = head;
            (head->next)->prev = nprev;
            head->next = nprev;
        }
    }
    if (left == left_stay) {
        (head->next)->prev = (right->prev)->prev;
        (right->prev)->prev = head->next;
        head->next = right;
        right->prev = head;
    } else {
        (head->next)->prev = (left->prev)->prev;
        (left->prev)->prev = head->next;
        head->next = left;
        left->prev = head;
    }
    return head;
}
void nf(struct list_head *head)
{
    printf("pf\r\n");
    struct list_head *h_n = head->next;
    while (h_n != head) {
        element_t *l_element = container_of(h_n, element_t, list);
        printf("%s->", l_element->value);
        h_n = h_n->next;
    }
    printf("\r\n");
}
void pf(struct list_head *head)
{
    printf("pf\r\n");
    struct list_head *h_n = head->prev;
    while (h_n != head) {
        element_t *l_element = container_of(h_n, element_t, list);
        printf("%s->", l_element->value);
        h_n = h_n->prev;
    }
    printf("\r\n");
}
struct list_head *m_sort(struct list_head *head)
{
    struct list_head *left, *right;
    if (!head || head->next == head) {
        printf("return head\r\n");
        return head;
    }
    printf("head\r\n");
    half(head, &left, &right);
    printf("head fffff\r\n");
    pf(right);
    nf(left);
    m_sort(left);
    m_sort(right);
    head = merge(left, right);
    return head;
}
void q_sort(struct list_head *head)
{
    if (!head || head->next == head) {
        printf("return head\r\n");
        return;
    }
    m_sort(head);
}
