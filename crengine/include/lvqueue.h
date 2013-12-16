#ifndef LVQUEUE_H_INCLUDED
#define LVQUEUE_H_INCLUDED

template < typename T >
class LVQueue {
    friend struct Iterator;
    struct Item {
        T value;
        Item * next;
        Item * prev;
        Item(T & v) : value(v), next(NULL), prev(NULL) {}
    };
    Item * head;
    Item * tail;
    int count;


    Item * remove(Item * p) {
        if (!p)
            return NULL;
        if (!p->prev)
            head = p->next;
        else
            p->prev->next = p->next;
        if (!p->next)
            tail = p->prev;
        else
            p->next->prev = p->prev;
        p->next = NULL;
        p->prev = NULL;
        count--;
        if (count == 0) {
        	head = tail = NULL;
        }

        return p;
    }
    void moveToHead(Item * item) {
        Item * p = remove(item);
        if (head) {
            head->prev = p;
            p->next = head;
            head = p;
        } else {
            head = tail = p;
        }
        count++;
    }
public:
    struct Iterator {
    private:
        LVQueue * queue;
        Item * currentItem;
    public:
        Iterator(const Iterator & v) {
            queue = v.queue;
            currentItem = v.currentItem;
        }
        Iterator(LVQueue * _queue) : queue(_queue), currentItem(NULL) {
        }
        T get() { return currentItem ? currentItem->value : T(); }
        void set(T value) { if (currentItem) currentItem->value = value; }
        bool next() {
            if (!currentItem) {
                // first time
                currentItem = queue->head;
            } else {
                // continue
                currentItem = currentItem->next;
            }
            return currentItem != NULL;
        }
        T remove() {
            if (!currentItem)
                return T();
            Item * next = currentItem->next;
            Item * p = queue->remove(currentItem);
            currentItem = next;
            T res = p->value;
            delete p;
            return res;
        }
        void moveToHead() {
            if (currentItem)
                queue->moveToHead(currentItem);
        }
    };

public:
    Iterator iterator() { return Iterator(this); }
    LVQueue() : head(NULL), tail(NULL), count(0) {}
    ~LVQueue() { clear(); }
//    T & operator [] (int index) {
//        Item * p = head;
//        for (int i = 0; i < index; i++) {
//            if (!p)
//                return
//        }
//    }

    int length() { return count; }
    void pushBack(T item) {
        Item * p = new Item(item);
        if (tail) {
            tail->next = p;
            p->prev = tail;
            tail = p;
        } else {
            head = tail = p;
        }
        count++;
    }
    void pushFront(T item) {
        Item * p = new Item(item);
        if (head) {
            head->prev = p;
            p->next = head;
            head = p;
        } else {
            head = tail = p;
        }
        count++;
    }
    T popFront() {
        if (!head)
            return T();
        Item * p = remove(head);
        T res = p->value;
        delete p;
        return res;
    }
    T popBack() {
        if (!tail)
            return T();
        Item * p = remove(tail);
        T res = p->value;
        delete p;
        return res;
    }
    void clear() {
        while (head) {
            Item * p = head;
            head = p->next;
            delete p;
        }
        head = NULL;
        tail = NULL;
        count = 0;
    }
};



#endif // LVQUEUE_H_INCLUDED
