#ifndef __LIB_KERNEL_HASH_H
#define __LIB_KERNEL_HASH_H

/* Hash table.
 *
 * This data structure is thoroughly documented in the Tour of
 * Pintos for Project 3.
 *
 * This is a standard hash table with chaining.  To locate an
 * element in the table, we compute a hash function over the
 * element's data and use that as an index into an array of
 * doubly linked lists, then linearly search the list.
 *
 * The chain lists do not use dynamic allocation.  Instead, each
 * structure that can potentially be in a hash must embed a
 * struct hash_elem member.  All of the hash functions operate on
 * these `struct hash_elem's.  The hash_entry macro allows
 * conversion from a struct hash_elem back to a structure object
 * that contains it.  This is the same technique used in the
 * linked list implementation.  Refer to lib/kernel/list.h for a
 * detailed explanation. */

/* 이는 체이닝 방식을 사용하는 표준 해시 테이블입니다. 테이블에서 요소를 찾으려면,
 * 요소의 데이터를 해시 함수에 넣어 해시값을 계산한 뒤, 해당 해시값을 이중 연결 리스트
 * 배열의 인덱스로 사용하고, 그 인덱스 위치의 리스트를 순차 탐색합니다. */
/* 체인 리스트는 동적 할당을 사용하지 않습니다. 대신, 해시에 포함될 수 있는 각 구조체는
 * 반드시 struct hash_elem 멤버를 포함해야 합니다. 모든 해시 함수는 이 `struct hash_elem`을
 * 기반으로 동작하며, hash_entry 매크로를 통해 struct hash_elem에서 이를 포함하고 있는
 * 실제 구조체 객체로 변환할 수 있습니다. 이 기법은 링크드 리스트 구현 시에도 사용된 것과
 * 동일합니다. 자세한 내용은 lib/kernel/list.h를 참고하세요. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "list.h"

/* Hash element. */
struct hash_elem {
	struct list_elem list_elem;
};

/* Converts pointer to hash element HASH_ELEM into a pointer to
 * the structure that HASH_ELEM is embedded inside.  Supply the
 * name of the outer structure STRUCT and the member name MEMBER
 * of the hash element.  See the big comment at the top of the
 * file for an example. */
/* 해시 요소 HASH_ELEM의 포인터를, 해당 HASH_ELEM을 포함하고 있는 상위 구조체의 포인터로 변환합니다.
 * 매크로 호출 시 상위 구조체 이름 STRUCT와 그 내부에 있는 해시 멤버 이름 MEMBER를 제공하세요.
 * 사용 예시와 자세한 설명은 파일 상단의 큰 주석을 참고하세요. */
#define hash_entry(HASH_ELEM, STRUCT, MEMBER)                   \
	((STRUCT *) ((uint8_t *) &(HASH_ELEM)->list_elem        \
		- offsetof (STRUCT, MEMBER.list_elem)))

/* Computes and returns the hash value for hash element E, given
 * auxiliary data AUX. */
/* 해시 요소 E에 대해 해시값을 계산하여 반환합니다. AUX는 보조 데이터를 의미합니다. */
typedef uint64_t hash_hash_func (const struct hash_elem *e, void *aux);

/* Compares the value of two hash elements A and B, given
 * auxiliary data AUX.  Returns true if A is less than B, or
 * false if A is greater than or equal to B. */
/* 해시 요소 A와 B를 AUX 보조 데이터를 사용하여 비교합니다. A가 B보다 작으면 true,
 * 그렇지 않으면(false 또는 같거나 크면) false를 반환합니다. */
typedef bool hash_less_func (const struct hash_elem *a,
		const struct hash_elem *b,
		void *aux);

/* Performs some operation on hash element E, given auxiliary
 * data AUX. */
/* 해시 요소 E에 대해 지정된 연산을 수행합니다. AUX는 보조 데이터를 의미합니다. */
typedef void hash_action_func (struct hash_elem *e, void *aux);

/* Hash table. */
struct hash {
	size_t elem_cnt;            /* Number of elements in table. */
	size_t bucket_cnt;          /* Number of buckets, a power of 2. */
	struct list *buckets;       /* Array of `bucket_cnt' lists. */
	hash_hash_func *hash;       /* Hash function. */
	hash_less_func *less;       /* Comparison function. */
	void *aux;                  /* Auxiliary data for `hash' and `less'. */
};

/* A hash table iterator. */
struct hash_iterator {
	struct hash *hash;          /* The hash table. */
	struct list *bucket;        /* Current bucket. */
	struct hash_elem *elem;     /* Current hash element in current bucket. */
};

/* Basic life cycle. */
/* 해시 테이블을 초기화합니다. */
bool hash_init (struct hash *, hash_hash_func *, hash_less_func *, void *aux);
/* 해시 테이블의 모든 요소를 제거하되, 해시 테이블 자체는 유지합니다.
 * 각 요소에 대해 hash_action_func를 호출합니다. */
void hash_clear (struct hash *, hash_action_func *);
/* 해시 테이블 자체와 모든 요소를 제거합니다.
 * 각 요소에 대해 hash_action_func를 호출하고, 할당된 리소스를 해제합니다. */
void hash_destroy (struct hash *, hash_action_func *);

/* Search, insertion, deletion. */
/* 해시 테이블에 새로운 요소를 삽입합니다. 중복 키가 있으면 삽입하지 않고 NULL을 반환합니다. */
struct hash_elem *hash_insert (struct hash *, struct hash_elem *);
/* 해시 테이블에 요소를 삽입하되, 키가 중복되면 기존 요소를 대체하고 기존 요소를 반환합니다. */
struct hash_elem *hash_replace (struct hash *, struct hash_elem *);
/* 해시 테이블에서 특정 요소를 검색합니다. 검색 결과를 반환하거나 NULL을 반환합니다. */
struct hash_elem *hash_find (struct hash *, struct hash_elem *);
/* 해시 테이블에서 특정 요소를 삭제하고, 삭제된 요소를 반환합니다. 없으면 NULL을 반환합니다. */
struct hash_elem *hash_delete (struct hash *, struct hash_elem *);

/* Iteration. */
/* 해시 테이블의 모든 요소에 대해 hash_action_func를 호출합니다. */
void hash_apply (struct hash *, hash_action_func *);
/* 이터레이터를 해시 테이블의 첫 번째 요소 위치로 초기화합니다. */
void hash_first (struct hash_iterator *, struct hash *);
/* 이터레이터를 다음 요소로 이동하고, 해당 요소를 반환합니다.
 * 더 이상 요소가 없으면 NULL을 반환합니다. */
struct hash_elem *hash_next (struct hash_iterator *);
/* 이터레이터가 가리키는 현재 요소를 반환합니다. */
struct hash_elem *hash_cur (struct hash_iterator *);

/* Information. */
/* 해시 테이블에 저장된 요소 개수를 반환합니다. */
size_t hash_size (struct hash *);
/* 해시 테이블이 비어 있으면 true를 반환합니다. */
bool hash_empty (struct hash *);

/* Sample hash functions. */
/* 바이트 배열에 대한 해시 값을 계산하여 반환합니다. */
uint64_t hash_bytes (const void *, size_t);
/* 문자열에 대한 해시 값을 계산하여 반환합니다. */
uint64_t hash_string (const char *);
/* 정수형에 대한 해시 값을 계산하여 반환합니다. */
uint64_t hash_int (int);

#endif /* lib/kernel/hash.h */
