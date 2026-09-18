#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

typedef struct {
    char *sender;
    char *subject;
    char *date;
} Email;

typedef struct {
    Email *data;
    int size;
    int capacity;
} MaxHeap;

void init_heap(MaxHeap* h) {
    h->capacity = 16;
    h->size = 0;
    h->data = (Email*)malloc(h->capacity * sizeof(Email));
}

void ensure_capacity(MaxHeap* h) {
    if (h->size >= h->capacity) {
        h->capacity *= 2;
        h->data = (Email*)realloc(h->data, h->capacity * sizeof(Email));
    }
}

int get_priority(const char* sender) {
    if (strcmp(sender, "Boss") == 0) return 5;
    if (strcmp(sender, "Subordinate") == 0) return 4;
    if (strcmp(sender, "Peer") == 0) return 3;
    if (strcmp(sender, "ImportantPerson") == 0) return 2;
    if (strcmp(sender, "OtherPerson") == 0) return 1;
    return 0;
}

int date_to_int(const char* d) {
    int m, day, y;
    sscanf(d, "%d-%d-%d", &m, &day, &y);
    return y * 10000 + m * 100 + day;
}

bool is_higher_priority(const Email* a, const Email* b) {
    int p_a = get_priority(a->sender);
    int p_b = get_priority(b->sender);
    if (p_a != p_b) return p_a > p_b;
    int d_a = date_to_int(a->date);
    int d_b = date_to_int(b->date);
    return d_a > d_b; // newer first
}

void swap_emails(Email* a, Email* b) {
    Email temp = *a;
    *a = *b;
    *b = temp;
}

void heapify_up(MaxHeap* h, int idx) {
    while (idx > 0) {
        int parent = (idx - 1) / 2;
        if (is_higher_priority(&h->data[idx], &h->data[parent])) {
            swap_emails(&h->data[idx], &h->data[parent]);
            idx = parent;
        } else break;
    }
}

void heapify_down(MaxHeap* h, int idx) {
    while (true) {
        int largest = idx;
        int left = 2 * idx + 1;
        int right = 2 * idx + 2;

        if (left < h->size && is_higher_priority(&h->data[left], &h->data[largest]))
            largest = left;
        if (right < h->size && is_higher_priority(&h->data[right], &h->data[largest]))
            largest = right;

        if (largest != idx) {
            swap_emails(&h->data[idx], &h->data[largest]);
            idx = largest;
        } else break;
    }
}

void insert(MaxHeap* h, Email e) {
    ensure_capacity(h);
    h->data[h->size] = e;
    heapify_up(h, h->size);
    h->size++;
}

Email extract_max(MaxHeap* h) {
    if (h->size == 0) return (Email){NULL, NULL, NULL};
    Email top = h->data[0];
    h->size--;
    if (h->size > 0) {
        h->data[0] = h->data[h->size];
        heapify_down(h, 0);
    }
    return top;
}

int get_size(MaxHeap* h) { return h->size; }

void free_email(Email e) {
    if (e.sender) free(e.sender);
    if (e.subject) free(e.subject);
    if (e.date) free(e.date);
}

int main() {
    MaxHeap heap;
    init_heap(&heap);

    char line[1024];
    while (fgets(line, sizeof(line), stdin)) {
        // Remove trailing newline/carriage return
        size_t len = strlen(line);
        while (len > 0 && (line[len-1] == '\n' || line[len-1] == '\r')) {
            line[--len] = '\0';
        }

        if (strncmp(line, "EMAIL ", 6) == 0) {
            char *rest = line + 6;
            char *c1 = strchr(rest, ',');
            if (!c1) continue; // malformed

            size_t sender_len = c1 - rest;
            char *sender_str = (char*)malloc(sender_len + 1);
            strncpy(sender_str, rest, sender_len);
            sender_str[sender_len] = '\0';

            char *sub_start = c1 + 1;
            char *c2 = strchr(sub_start, ',');
            if (!c2) continue; // malformed

            size_t sub_len = c2 - sub_start;
            char *subject_str = (char*)malloc(sub_len + 1);
            strncpy(subject_str, sub_start, sub_len);
            subject_str[sub_len] = '\0';

            char *date_str = strdup(c2 + 1); // strdup is POSIX but widely available. Fallback if needed? I'll use malloc+strcpy for strict C.
            size_t date_len = strlen(c2 + 1);
            date_str = (char*)malloc(date_len + 1);
            strcpy(date_str, c2 + 1);

            Email e = {sender_str, subject_str, date_str};
            insert(&heap, e);
        } else if (strcmp(line, "NEXT") == 0) {
            if (get_size(&heap) > 0) {
                printf("Next email:\n");
                printf("\t Sender: %s\n", heap.data[0].sender);
                printf("\t Subject: %s\n", heap.data[0].subject);
                printf("\t Date: %s\n", heap.data[0].date);
            }
        } else if (strcmp(line, "READ") == 0) {
            if (get_size(&heap) > 0) {
                Email e = extract_max(&heap);
                free_email(e);
            }
        } else if (strcmp(line, "COUNT") == 0) {
            printf("There are %d emails to read.\n", get_size(&heap));
        }
    }

    // Cleanup remaining heap
    for (int i = 0; i < heap.size; i++) free_email(heap.data[i]);
    free(heap.data);

    return 0;
}
