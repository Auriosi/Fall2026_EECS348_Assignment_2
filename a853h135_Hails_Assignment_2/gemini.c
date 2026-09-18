/*
Name: EECS 348 Assignment 2 - Email Priority Queue Max-Heap
Description: Implements an email inbox priority queue using a dynamic binary Max-Heap.
    Emails are prioritized based on sender hierarchy, arrival date (newest first), and
    arrival order. Supports commands to add emails (EMAIL), view the next email (NEXT),
    read/remove the next email (READ), and display the unread email count (COUNT).
Input: Optional command-line argument specifying an input text file path; otherwise, reads
    commands interactively or via redirection from standard input (stdin).
Output: Standard output displaying the number of unread emails, formatted details of the
    highest-priority email, and error/warning messages for malformed commands.
Collaborators: Gemini
Author: Aaron Hails
Creation: 9/17/2026
Revision: 9/17/2026
Revision History: Initial creation.
    Modifications to improve correctness, execution speed, space efficiency, and maintainability.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Written by Gemini
#define MAX_SUBJECT_LEN 256
#define INITIAL_CAPACITY 16

// Written by Aaron Hails
typedef enum {
    UNKNOWN = 0,
    OTHER = 1,
    IMPORTANT,
    PEER,
    SUBORDINATE,
    BOSS
} Category;

// Written by Gemini
typedef struct {
    int month;
    int day;
    int year;
} Date;

// Written by Gemini, modified by Aaron Hails
typedef struct {
    Category sender_category;
    char subject[MAX_SUBJECT_LEN];
    Date date;
    int order;
} Email;

// Written by Gemini
typedef struct {
    Email *array;
    int capacity;
    int size;
} MaxHeap;

// Written by Gemini
static void trim(char *str) {
    if (!str) return; // Guard check against a null pointer

    char *start = str; // Start scanning at the beginning of the string
    while (*start && isspace((unsigned char)*start)) { // Advance pointer past any leading whitespace
        start++;
    }
    if (start != str) { // If leading whitespace was found, shift the entire string forward
        memmove(str, start, strlen(start) + 1);
    }

    int len = (int)strlen(str); // Obtain the current length of the shifted string
    while (len > 0 && isspace((unsigned char)str[len - 1])) { // Work backwards to strip trailing whitespace
        str[len - 1] = '\0'; // Replace trailing whitespace with a null terminator
        len--; // Decrement length index
    }
}

// Written by Gemini, modified by Gemini
static Date parseDate(const char *dateStr) {
    Date d = {0, 0, 0}; // Initialize date struct with default zeroes
    if (sscanf(dateStr, "%d-%d-%d", &d.month, &d.day, &d.year) != 3) { // Parse month, day, and year
        fprintf(stderr, "Warning: Malformed date '%s'\n", dateStr); // Warn if date string was not in MM-DD-YYYY format
    }
    return d; // Return the parsed date
}

// Written by Gemini
static int compareDates(Date d1, Date d2) {
    if (d1.year != d2.year) { // Check if years differ first
        return d1.year - d2.year;
    }
    if (d1.month != d2.month) { // Years are identical, check if months differ
        return d1.month - d2.month;
    }
    return d1.day - d2.day; // Months are identical, compare days
}

// Written by Gemini, modified by Aaron Hails
static int compareEmails(const Email *a, const Email *b) {
    int priA = a->sender_category; // Get numeric priority rank of email a
    int priB = b->sender_category; // Get numeric priority rank of email b
    if (priA != priB) { // Check if sender priorities are different
        return priA - priB; // Higher rank wins
    }

    /* Tie-breaker: newest email first */
    int dateComparison = compareDates(a->date, b->date); // Compare dates chronologically
    if (dateComparison != 0) { // If dates differ, use date as the tie-breaker
        return dateComparison;
    }

    /* Secondary tie-breaker: preserve earlier arrival */
    return b->order - a->order; // Earlier arrival has a smaller order value, so it has higher priority
}

// Written by Gemini
MaxHeap* createHeap(int initialCapacity) {
    MaxHeap *heap = (MaxHeap *)malloc(sizeof(MaxHeap)); // Allocate memory for the heap control struct
    if (!heap) { // Ensure allocation succeeded
        perror("Failed to allocate memory for heap");
        exit(EXIT_FAILURE);
    }
    heap->capacity = initialCapacity > 0 ? initialCapacity : INITIAL_CAPACITY; // Use initial capacity or fallback to 16
    heap->size = 0; // Initialize heap size to 0 elements
    heap->array = (Email *)malloc(sizeof(Email) * heap->capacity); // Allocate backing array storage
    if (!heap->array) { // Ensure array allocation succeeded
        perror("Failed to allocate memory for heap array");
        free(heap); // Free parent struct before exiting to prevent memory leaks
        exit(EXIT_FAILURE);
    }
    return heap; // Return pointer to the initialized heap
}

// Written by Gemini
static void swapEmails(Email *a, Email *b) {
    Email temp = *a; // Copy email a to temporary storage
    *a = *b;         // Overwrite email a with email b
    *b = temp;       // Overwrite email b with temporary email
}

// Written by Gemini
void heapifyUp(MaxHeap *heap, int index) {
    while (index > 0) { // Continue bubbling up until root index (0) is reached
        int parent = (index - 1) / 2; // Calculate parent index in a binary heap
        if (compareEmails(&heap->array[index], &heap->array[parent]) > 0) { // If child has higher priority than parent
            swapEmails(&heap->array[index], &heap->array[parent]); // Swap child and parent elements
            index = parent; // Move up to parent's position
        } else {
            break; // Max-heap property is satisfied
        }
    }
}

// Written by Gemini
void heapifyDown(MaxHeap *heap, int index) {
    while (1) {
        int left = 2 * index + 1;  // Calculate left child index
        int right = 2 * index + 2; // Calculate right child index
        int largest = index;       // Assume current index is the largest priority initially

        if (left < heap->size && compareEmails(&heap->array[left], &heap->array[largest]) > 0) { // Left child is higher priority
            largest = left;
        }
        if (right < heap->size && compareEmails(&heap->array[right], &heap->array[largest]) > 0) { // Right child is higher priority
            largest = right;
        }

        if (largest != index) { // A child has higher priority than current node
            swapEmails(&heap->array[index], &heap->array[largest]); // Swap current with larger child
            index = largest; // Move down into child's position
        } else {
            break; // Max-heap property is satisfied
        }
    }
}

// Written by Gemini
void insertHeap(MaxHeap *heap, Email email) {
    if (heap->size == heap->capacity) { // Heap array is full, double its capacity
        heap->capacity *= 2;
        Email *newArray = (Email *)realloc(heap->array, sizeof(Email) * heap->capacity); // Reallocate memory
        if (!newArray) { // Ensure realloc succeeded
            perror("Failed to resize heap array");
            exit(EXIT_FAILURE);
        }
        heap->array = newArray; // Point to the new array buffer
    }

    heap->array[heap->size] = email; // Insert new element at the end of the array
    heapifyUp(heap, heap->size);     // Bubble the new element up to its proper spot
    heap->size++;                    // Increment total count of stored emails
}

// Written by Gemini
int peekHeap(const MaxHeap *heap, Email *result) {
    if (heap->size == 0) { // Heap is empty
        return 0;
    }
    if (result) { // Copy root element to result pointer if non-null
        *result = heap->array[0];
    }
    return 1; // Return success status
}

// Written by Gemini
int popHeap(MaxHeap *heap, Email *result) {
    if (heap->size == 0) { // Heap is empty, cannot pop
        return 0;
    }
    if (result) { // Copy top element to result pointer if non-null
        *result = heap->array[0];
    }

    heap->array[0] = heap->array[heap->size - 1]; // Move last element to the root position
    heap->size--; // Decrement size

    if (heap->size > 0) { // If elements still remain, sift new root down
        heapifyDown(heap, 0);
    }
    return 1; // Return success status
}

// Written by Gemini
void freeHeap(MaxHeap *heap) {
    if (heap) {
        if (heap->array) {
            free(heap->array); // Free the underlying email array
        }
        free(heap); // Free the heap container struct itself
    }
}

// Written by Gemini, modified by Aaron Hails
static int addEmailLine(char *args, int order, Email *out_email) {
    // Split fields by comma: sender,subject,date
    char *first_comma = strchr(args, ','); // Search for first comma
    if (!first_comma) {
        return 1; // Missing comma delimiter
    }
    *first_comma = '\0'; // Split string at first comma

    char *second_comma = strchr(first_comma + 1, ','); // Search for second comma
    if (!second_comma) {
        return 1; // Missing second comma delimiter
    }
    *second_comma = '\0'; // Split string at second comma

    char *raw_sender  = args;             // Start of sender token
    char *raw_subject = first_comma + 1;  // Start of subject token
    char *raw_date    = second_comma + 1; // Start of date token

    trim(raw_sender);  // Strip whitespace from sender token
    trim(raw_subject); // Strip whitespace from subject token
    trim(raw_date);    // Strip whitespace from date token

    // Convert sender name into Category enum for fast integer comparisons
    if (strcmp(raw_sender, "Boss") == 0) out_email->sender_category = BOSS;
    else if (strcmp(raw_sender, "Subordinate") == 0) out_email->sender_category = SUBORDINATE;
    else if (strcmp(raw_sender, "Peer") == 0) out_email->sender_category = PEER;
    else if (strcmp(raw_sender, "ImportantPerson") == 0) out_email->sender_category = IMPORTANT;
    else if (strcmp(raw_sender, "OtherPerson") == 0) out_email->sender_category = OTHER;
    else out_email->sender_category = UNKNOWN;

    snprintf(out_email->subject, sizeof(out_email->subject), "%s", raw_subject); // Store subject safely
    out_email->date = parseDate(raw_date);  // Populate the date field in out_email
    out_email->order = order; // Assign arrival order sequence

    return 0; // Return success
}

// Written by Gemini, modified by Aaron Hails
static const char* formatSenderName(Category cat) {
    switch (cat) { // Map category enum to printable string
        case BOSS:        return "Boss";
        case SUBORDINATE: return "Subordinate";
        case PEER:        return "Peer";
        case IMPORTANT:   return "ImportantPerson";
        case OTHER:       return "OtherPerson";
        case UNKNOWN:
        default:          return "Unknown";
    }
}

// Written by Gemini, modified by Aaron Hails
int main(int argc, char *argv[]) {
    FILE *fp = stdin; // Default input source is standard input
    if (argc > 1) { // A file path argument was provided
        fp = fopen(argv[1], "r"); // Attempt to open the input file
        if (!fp) { // File could not be opened
            perror("Error opening input file");
            return EXIT_FAILURE;
        }
    }

    MaxHeap *heap = createHeap(INITIAL_CAPACITY); // Initialize heap with initial capacity of 16
    char line[1024]; // Buffer to hold each input line
    int arrivalOrder = 0; // Running arrival counter to break ties for identical priority/date

    while (fgets(line, sizeof(line), fp)) { // Read line-by-line until EOF
        line[strcspn(line, "\r\n")] = '\0'; // Remove trailing newlines or carriage returns

        char *cmd = line; // Pointer to command start
        while (*cmd && isspace((unsigned char)*cmd)) { // Skip any leading whitespace
            cmd++;
        }
        if (*cmd == '\0') { // Skip empty lines
            continue;
        }

        if (strncmp(cmd, "EMAIL", 5) == 0 && (cmd[5] == ' ' || cmd[5] == '\t')) { // Check for EMAIL command
            Email newEmail; // Temporary variable to store parsed email
            if (addEmailLine(cmd + 5, arrivalOrder, &newEmail) == 0) { // Parse remaining arguments
                insertHeap(heap, newEmail); // Insert newly created email into max-heap
                arrivalOrder++; // Increment arrival sequence counter
            } else {
                fprintf(stderr, "Warning: Malformed EMAIL command: '%s'\n", line); // Report malformed line
            }
        } else if (strcmp(cmd, "COUNT") == 0) { // Check for COUNT command
            printf("There are %d emails to read.\n", heap->size); // Print current count of emails
        } else if (strcmp(cmd, "NEXT") == 0) { // Check for NEXT command
            Email nextEmail; // Temporary variable to store top email
            if (peekHeap(heap, &nextEmail)) { // Peek root element without removing it
                printf("Next email:\n");
                printf("\t Sender: %s\n", formatSenderName(nextEmail.sender_category));
                printf("\t Subject: %s\n", nextEmail.subject);
                printf("\t Date: %02d-%02d-%04d\n", nextEmail.date.month, nextEmail.date.day, nextEmail.date.year);
            }
        } else if (strcmp(cmd, "READ") == 0) { // Check for READ command
            popHeap(heap, NULL); // Remove highest priority email from the heap
        } else {
            printf("Warning: Unknown command '%s'\n", cmd); // Handle unknown commands
        }
    }

    freeHeap(heap); // Deallocate all heap memory
    if (fp != stdin) { // If input was read from a file, close the file handle
        fclose(fp);
    }

    return EXIT_SUCCESS; // Program completed successfully
}