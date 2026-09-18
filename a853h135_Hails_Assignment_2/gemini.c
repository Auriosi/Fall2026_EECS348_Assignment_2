#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_SUBJECT_LEN 256
#define INITIAL_CAPACITY 16

typedef enum {
    UNKNOWN = 0,
    OTHER = 1,
    IMPORTANT,
    PEER,
    SUBORDINATE,
    BOSS
} Category;

typedef struct {
    int month;
    int day;
    int year;
} Date;

typedef struct {
    Category sender_category;
    char subject[MAX_SUBJECT_LEN];
    Date date;
    int order;
} Email;

typedef struct {
    Email *array;
    int capacity;
    int size;
} MaxHeap;

/* Helper string trimming function */
static void trim(char *str) {
    if (!str) return;

    char *start = str;
    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != str) {
        memmove(str, start, strlen(start) + 1);
    }

    int len = (int)strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[len - 1] = '\0';
        len--;
    }
}

/* Parse date from format MM-DD-YYYY */
static Date parseDate(const char *dateStr) {
    Date d = {0, 0, 0};
    if (sscanf(dateStr, "%d-%d-%d", &d.month, &d.day, &d.year) != 3) {
        fprintf(stderr, "Warning: Malformed date '%s'\n", dateStr);
    }
    return d;
}

/* Compare dates: returns > 0 if d1 is newer than d2, < 0 if older, 0 if identical */
static int compareDates(Date d1, Date d2) {
    if (d1.year != d2.year) {
        return d1.year - d2.year;
    }
    if (d1.month != d2.month) {
        return d1.month - d2.month;
    }
    return d1.day - d2.day;
}

/* Compare two emails: returns > 0 if a has higher priority than b, < 0 if lower */
static int compareEmails(const Email *a, const Email *b) {
    int priA = a->sender_category;
    int priB = b->sender_category;
    if (priA != priB) {
        return priA - priB;
    }

    /* Tie-breaker: newest email first */
    int dateComparison = compareDates(a->date, b->date);
    if (dateComparison != 0) {
        return dateComparison;
    }

    /* Secondary tie-breaker: preserve earlier arrival */
    return b->order - a->order;
}

/* Create and initialize an empty MaxHeap */
MaxHeap* createHeap(int initialCapacity) {
    MaxHeap *heap = (MaxHeap *)malloc(sizeof(MaxHeap));
    if (!heap) {
        perror("Failed to allocate memory for heap");
        exit(EXIT_FAILURE);
    }
    heap->capacity = initialCapacity > 0 ? initialCapacity : INITIAL_CAPACITY;
    heap->size = 0;
    heap->array = (Email *)malloc(sizeof(Email) * heap->capacity);
    if (!heap->array) {
        perror("Failed to allocate memory for heap array");
        free(heap);
        exit(EXIT_FAILURE);
    }
    return heap;
}

/* Swap two Email elements */
static void swapEmails(Email *a, Email *b) {
    Email temp = *a;
    *a = *b;
    *b = temp;
}

/* Sift up an element to maintain the max-heap property */
void heapifyUp(MaxHeap *heap, int index) {
    while (index > 0) {
        int parent = (index - 1) / 2;
        if (compareEmails(&heap->array[index], &heap->array[parent]) > 0) {
            swapEmails(&heap->array[index], &heap->array[parent]);
            index = parent;
        } else {
            break;
        }
    }
}

/* Sift down an element to maintain the max-heap property */
void heapifyDown(MaxHeap *heap, int index) {
    while (1) {
        int left = 2 * index + 1;
        int right = 2 * index + 2;
        int largest = index;

        if (left < heap->size && compareEmails(&heap->array[left], &heap->array[largest]) > 0) {
            largest = left;
        }
        if (right < heap->size && compareEmails(&heap->array[right], &heap->array[largest]) > 0) {
            largest = right;
        }

        if (largest != index) {
            swapEmails(&heap->array[index], &heap->array[largest]);
            index = largest;
        } else {
            break;
        }
    }
}

/* Insert a new email into the MaxHeap */
void insertHeap(MaxHeap *heap, Email email) {
    if (heap->size == heap->capacity) {
        heap->capacity *= 2;
        Email *newArray = (Email *)realloc(heap->array, sizeof(Email) * heap->capacity);
        if (!newArray) {
            perror("Failed to resize heap array");
            exit(EXIT_FAILURE);
        }
        heap->array = newArray;
    }

    heap->array[heap->size] = email;
    heapifyUp(heap, heap->size);
    heap->size++;
}

/* Inspect the highest priority email without removing it */
int peekHeap(const MaxHeap *heap, Email *result) {
    if (heap->size == 0) {
        return 0;
    }
    if (result) {
        *result = heap->array[0];
    }
    return 1;
}

/* Remove and return the highest priority email */
int popHeap(MaxHeap *heap, Email *result) {
    if (heap->size == 0) {
        return 0;
    }
    if (result) {
        *result = heap->array[0];
    }

    heap->array[0] = heap->array[heap->size - 1];
    heap->size--;

    if (heap->size > 0) {
        heapifyDown(heap, 0);
    }
    return 1;
}

/* Free all memory associated with the MaxHeap */
void freeHeap(MaxHeap *heap) {
    if (heap) {
        if (heap->array) {
            free(heap->array);
        }
        free(heap);
    }
}

static int addEmailLine(char *args, int order, Email *out_email) {
    // Split fields by comma: sender,subject,date
    char *first_comma = strchr(args, ',');
    if (!first_comma) {
        return 1;
    }
    *first_comma = '\0';

    char *second_comma = strchr(first_comma + 1, ',');
    if (!second_comma) {
        return 1;
    }
    *second_comma = '\0';

    char *raw_sender  = args;
    char *raw_subject = first_comma + 1;
    char *raw_date    = second_comma + 1;

    trim(raw_sender);
    trim(raw_subject);
    trim(raw_date);

    // Populate string buffers safely
    if (strcmp(raw_sender, "Boss") == 0) out_email->sender_category = BOSS;
    else if (strcmp(raw_sender, "Subordinate") == 0) out_email->sender_category = SUBORDINATE;
    else if (strcmp(raw_sender, "Peer") == 0) out_email->sender_category = PEER;
    else if (strcmp(raw_sender, "ImportantPerson") == 0) out_email->sender_category = IMPORTANT;
    else if (strcmp(raw_sender, "OtherPerson") == 0) out_email->sender_category = OTHER;
    else out_email->sender_category = UNKNOWN;
    snprintf(out_email->subject, sizeof(out_email->subject), "%s", raw_subject);
    out_email->date = parseDate(raw_date);  // This will populate the date field in out_email
    out_email->order = order;

    return 0;
}

static const char* formatSenderName(Category cat) {
    switch (cat) {
        case BOSS:        return "Boss";
        case SUBORDINATE: return "Subordinate";
        case PEER:        return "Peer";
        case IMPORTANT:   return "ImportantPerson";
        case OTHER:       return "OtherPerson";
        case UNKNOWN:
        default:               return "Unknown";
    }
}

int main(int argc, char *argv[]) {
    FILE *fp = stdin;
    if (argc > 1) {
        fp = fopen(argv[1], "r");
        if (!fp) {
            perror("Error opening input file");
            return EXIT_FAILURE;
        }
    }

    MaxHeap *heap = createHeap(INITIAL_CAPACITY);
    char line[1024];
    int arrivalOrder = 0;

    while (fgets(line, sizeof(line), fp)) {
        line[strcspn(line, "\r\n")] = '\0';

        char *cmd = line;
        while (*cmd && isspace((unsigned char)*cmd)) {
            cmd++;
        }
        if (*cmd == '\0') {
            continue;
        }

        if (strncmp(cmd, "EMAIL", 5) == 0 && (cmd[5] == ' ' || cmd[5] == '\t')) {
            Email newEmail;
            if (addEmailLine(cmd + 5, arrivalOrder++, &newEmail) == 0) {
                insertHeap(heap, newEmail);
                arrivalOrder++;
            } else {
                fprintf(stderr, "Warning: Malformed EMAIL command: '%s'\n", line);
            }
        } else if (strcmp(cmd, "COUNT") == 0) {
            printf("There are %d emails to read.\n", heap->size);
        } else if (strcmp(cmd, "NEXT") == 0) {
            Email nextEmail;
            if (peekHeap(heap, &nextEmail)) {
                printf("Next email:\n");
                printf("\t Sender: %s\n", formatSenderName(nextEmail.sender_category));
                printf("\t Subject: %s\n", nextEmail.subject);
                printf("\t Date: %02d-%02d-%04d\n", nextEmail.date.month, nextEmail.date.day, nextEmail.date.year);
            }
        } else if (strcmp(cmd, "READ") == 0) {
            popHeap(heap, NULL);
        } else {
            printf("Warning: Unknown command '%s'\n", cmd);
        }
    }

    freeHeap(heap);
    if (fp != stdin) {
        fclose(fp);
    }

    return EXIT_SUCCESS;
}