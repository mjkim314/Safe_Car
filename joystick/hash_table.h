#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "header.h"

#define TABLE_SIZE 3 // �ؽ� ���̺� ũ�� (�Ҽ� ��� ����)

// ��� ����ü (ü�̴��� ���� ���� ����Ʈ)
typedef struct s_node {
    char* key;              // ���ڿ� Ű
    int sock;               // ���� ��ȣ
    struct s_node* next;    // ���� ��� (ü�̴׿�)
} t_node;

// �ؽ� ���̺� ����ü
typedef struct {
    t_node* buckets[TABLE_SIZE]; // �ؽ� ���̺��� ��Ŷ �迭
    int count;                   // ����� �׸� ��
} t_hash_table;

// DJB2 �ؽ� �Լ�
unsigned long hash_func(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % TABLE_SIZE;
}

// �ؽ� ���̺� ����
t_hash_table* create_table() {
    t_hash_table* table = (t_hash_table*)malloc(sizeof(t_hash_table));
    if (!table) {
        perror("Failed to allocate memory for hash table");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < TABLE_SIZE; i++) {
        table->buckets[i] = NULL;
    }
    table->count = 0;
    return table;
}

// ��� ����
t_node* create_node(const char* key, int sock) {
    t_node* node = (t_node*)malloc(sizeof(t_node));
    if (!node) {
        perror("Failed to allocate memory for node");
        exit(EXIT_FAILURE);
    }
    node->key = strdup(key);   // ���ڿ� ����
    node->sock = sock;         // ���� ��ȣ ����
    node->next = NULL;
    return node;
}

// ������ �߰�
void add_to_table(t_hash_table* table, const char* key, int sock) {
    unsigned long index = hash_func(key);
    t_node* new_node = create_node(key, sock);

    // �ش� ��Ŷ�� ��� �߰� (ü�̴�)
    if (!table->buckets[index]) {
        table->buckets[index] = new_node;
    }
    else {
        // ü�̴��� ���� ���� ����Ʈ�� �� �տ� �߰�
        new_node->next = table->buckets[index];
        table->buckets[index] = new_node;
    }
    table->count++;
}

// ������ �˻�
bool search_table(t_hash_table* table, const char* key) {
    unsigned long index = hash_func(key);
    t_node* current = table->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            return true; // �ش� Ű�� �����ϸ� true ��ȯ
        }
        current = current->next;
    }
    return false; // �ش� Ű�� ������ false ��ȯ
}

// ������ ����
bool remove_from_table(t_hash_table* table, const char* key) {
    unsigned long index = hash_func(key);
    t_node* current = table->buckets[index];
    t_node* prev = NULL;

    while (current) {
        if (strcmp(current->key, key) == 0) {
            if (prev) {
                prev->next = current->next;
            }
            else {
                table->buckets[index] = current->next;
            }
            free(current->key);
            free(current);
            table->count--;
            return true;
        }
        prev = current;
        current = current->next;
    }
    return false;
}

// Ű�� �Ű������� �ް� �ش� Ű�� sock ���� ��ȯ�ϴ� �Լ�
int get_sock_by_key(t_hash_table* table, const char* key) {
    unsigned long index = hash_func(key);
    t_node* current = table->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->sock; // �ش� Ű�� �����ϴ� sock ��ȯ
        }
        current = current->next;
    }
    // Ű�� �������� ������ -1 (���� �ڵ�) ��ȯ
    return -1;
}

// �ؽ� ���̺� �� Ű ������ ����ϴ� �Լ�
void print_clients(t_hash_table* table) {

    printf("\nConnected Clients\n");

    // �ؽ� ���̺��� �� ��Ŷ�� Ȯ��
    for (int i = 0; i < TABLE_SIZE; i++) {
        t_node* current = table->buckets[i];

        // �� ��Ŷ ���� ���� ���
        while (current) {
            printf("%s\n", current->key);  // �ִ� 48���ڱ��� ���߱�
            current = current->next;
        }
    }
    printf("\n");
}

// �ؽ� ���̺� �޸� ����
void free_table(t_hash_table* table) {
    for (int i = 0; i < TABLE_SIZE; i++) {
        t_node* current = table->buckets[i];
        while (current) {
            t_node* temp = current;
            current = current->next;
            free(temp->key);
            free(temp);
        }
    }
    free(table);
}

#endif // HASH_TABLE_H
