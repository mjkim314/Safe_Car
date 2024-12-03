#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "header.h"
#include "lcd.h"

#define TABLE_SIZE 3 // 해시 테이블 크기 (소수 사용 권장)

// 노드 구조체 (체이닝을 위한 연결 리스트)
typedef struct s_node {
    char* key;              // 문자열 키
    int sock;               // 소켓 번호
    struct s_node* next;    // 다음 노드 (체이닝용)
} t_node;

// 해시 테이블 구조체
typedef struct {
    t_node* buckets[TABLE_SIZE]; // 해시 테이블의 버킷 배열
    int count;                   // 저장된 항목 수
} t_hash_table;

// DJB2 해시 함수
unsigned long hash_func(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash % TABLE_SIZE;
}

// 해시 테이블 생성
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

// 노드 생성
t_node* create_node(const char* key, int sock) {
    t_node* node = (t_node*)malloc(sizeof(t_node));
    if (!node) {
        perror("Failed to allocate memory for node");
        exit(EXIT_FAILURE);
    }
    node->key = strdup(key);   // 문자열 복사
    node->sock = sock;         // 소켓 번호 저장
    node->next = NULL;
    return node;
}

// 데이터 추가
void add_to_table(t_hash_table* table, const char* key, int sock) {
    unsigned long index = hash_func(key);
    t_node* new_node = create_node(key, sock);

    // 해당 버킷에 노드 추가 (체이닝)
    if (!table->buckets[index]) {
        table->buckets[index] = new_node;
    }
    else {
        // 체이닝을 위해 연결 리스트의 맨 앞에 추가
        new_node->next = table->buckets[index];
        table->buckets[index] = new_node;
    }
    table->count++;
}

// 데이터 검색
bool search_table(t_hash_table* table, const char* key) {
    unsigned long index = hash_func(key);
    t_node* current = table->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            return true; // 해당 키가 존재하면 true 반환
        }
        current = current->next;
    }
    return false; // 해당 키가 없으면 false 반환
}

// 데이터 삭제
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

// 키를 매개변수로 받고 해당 키의 sock 값을 반환하는 함수
int get_sock_by_key(t_hash_table* table, const char* key) {
    unsigned long index = hash_func(key);
    t_node* current = table->buckets[index];

    while (current) {
        if (strcmp(current->key, key) == 0) {
            return current->sock; // 해당 키에 대응하는 sock 반환
        }
        current = current->next;
    }
    // 키가 존재하지 않으면 -1 (에러 코드) 반환
    return -1;
}
<<<<<<< HEAD
/*
=======
// 해시 테이블 내 키 값들을 출력하는 함수
>>>>>>> 1d9b7b01a19cfb8bd00691251f8e078654fd2285
void print_clients(t_hash_table* table) {
    printf("\nConnected Clients\n");
    // 해시 테이블의 각 버킷을 확인
    for (int i = 0; i < TABLE_SIZE; i++) {
        t_node* current = table->buckets[i];
        // 각 버킷 내의 노드들 출력
        while (current) {
            printf("%s\n", current->key);  // 최대 48글자까지 맞추기
            current = current->next;
        }
    }
    printf("\n");
}
*/

// 해시 테이블 메모리 해제
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
