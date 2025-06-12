/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "vm/vm.h"
#include "userprog/process.h"
#include "vm/inspect.h"
#include "threads/mmu.h"
#include "filesys/file.h"

static struct list frame_table;
static struct lock frame_lock;

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
// 각 하위 시스템의 초기화 코드를 호출하여 가상 메모리 하위 시스템을 초기화합니다.
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
	list_init(&frame_table);
	lock_init(&frame_lock);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
/* 페이지의 타입을 가져옵니다. 페이지가 초기화된 후의 타입을 알고 싶을 때 유용합니다.
 * 이 함수는 현재 완전히 구현되어 있습니다. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
/* 초기화자와 함께 대기 중인 페이지 객체를 생성합니다. 페이지를 생성하려면,
 * 직접 생성하지 말고 이 함수나 `vm_alloc_page`를 통해 생성하세요. */


bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {
	ASSERT (VM_TYPE(type) != VM_UNINIT)
	// printf("vm alloc page\n");
	struct supplemental_page_table *spt = &thread_current ()->spt;
	
	void *va = pg_round_down(upage);
	// printf("[vm_alloc_page_with_initializer] va : %p\n", va);
	// printf("[vm_alloc_page_with_initializer] upage : %p\n", upage);
	/* upage가 이미 점유되어 있는지 확인합니다. */
	if (spt_find_page (spt, va) == NULL) {
		/* TODO: 페이지를 생성하고, VM 타입에 따라 초기화자를 가져온 후,
		 * TODO: uninit_new를 호출하여 '언인트(uninit)' 페이지 구조체를 생성하세요.
		 * TODO: uninit_new를 호출한 후 필드를 수정해야 합니다. */

		/* TODO: 페이지를 spt에 삽입하세요. */
		struct page *pg = calloc(1, sizeof(struct page));
		if(pg == NULL){
			return NULL;
		}
		bool initializer = false;
		if(VM_TYPE(type) == VM_ANON){
			uninit_new(pg, va, init, type, aux, anon_initializer);
		}
		else if(VM_TYPE(type) == VM_FILE){
			uninit_new(pg, va, init, type, aux, file_backed_initializer);
		}
		else{
			// printf("VM_ERROR\n");
			return false;
		}
		pg ->writable = writable;

		return spt_insert_page(spt, pg);
	}
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
/* spt에서 VA를 찾아 페이지를 반환합니다. 오류 시 NULL을 반환합니다. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function. */
	struct page tmp;
	tmp.va = pg_round_down(va);

	struct hash_elem *search_elem = hash_find(&spt->spt, &tmp.h_elem);
	if(search_elem == NULL)
		return NULL;
	page = hash_entry(search_elem, struct page, h_elem);
	return page;
}

/* Insert PAGE into spt with validation. */
/* 유효성 검사를 통해 PAGE를 spt에 삽입합니다. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
	
	struct hash_elem *insert_hash_elem = hash_insert(spt, &page->h_elem);

	if(insert_hash_elem == NULL)
		succ = true;
	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
	struct hash_elem h = page->h_elem;
	hash_delete(spt, &h);
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
/* 교체될 프레임 구조체를 가져옵니다. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */
	/* TODO: 교체 정책은 여러분에게 달려 있습니다. */

	struct thread *curr = thread_current();
	struct list_elem *e;

	for(e = list_front(&frame_table); e != list_end(&frame_table); e = list_next(&frame_table)){
		victim = list_entry(e, struct frame, frame_elem);
		if(pml4_is_accessed(curr->pml4, victim->page->va)){
			pml4_set_accessed(curr->pml4,  victim->page->va, 0);
		}
		else return victim;
	}
	for(e = list_front(&frame_table); e != list_end(&frame_table); e = list_next(&frame_table)){
		victim = list_entry(e, struct frame, frame_elem);
		return victim;
	}
	return NULL;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
/* 하나의 페이지를 교체(이젝트)하고 해당 프레임을 반환합니다.
 * 오류 시 NULL을 반환합니다. */
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

	return NULL;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
/* palloc()을 사용하여 프레임을 가져옵니다. 사용 가능한 페이지가 없으면,
 * 페이지를 교체(이젝트)하고 반환합니다. 이 함수는 항상 유효한 주소를 반환합니다.
 * 즉, 유저 풀 메모리가 가득 차 있으면, 이 함수는 교체를 통해 사용 가능한 메모리
 * 공간을 확보합니다. */
static struct frame *vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */
	frame = malloc(sizeof (struct frame)); /* 먼저 프레임 타입의 공간을 따로 잡아준다. */
	if(frame == NULL) return NULL;

	frame->kva = palloc_get_page(PAL_USER); /* 실제 물리 페이지의 주소를 할당해준다. */
	if(frame->kva == NULL){
		/* 미완성 */
		/*
		lock_acquire(&frame_lock);
		frame = vm_evict_frame();
		lock_release(&frame_lock);
		*/
		
		free(frame);
		PANIC ("todo");
		return NULL;
		
		if(frame == NULL) return NULL;
	}
	else{
		frame->page = NULL;
		lock_acquire(&frame_lock);
		list_push_back(&frame_table, &frame->frame_elem);
		lock_release(&frame_lock);
	}
	return frame;
}

/* Growing the stack. */
/* 스택 확장. */
static void
vm_stack_growth (void *addr UNUSED) {
	vm_alloc_page(VM_ANON, addr, true);
}

/* Handle the fault on write_protected page */
/* 쓰기 보호된 페이지에서 발생한 폴트를 처리합니다. */
static bool
vm_handle_wp (struct page *page UNUSED) {
	
}

/* Return true on success */
/* 성공 시 true를 반환합니다. */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
	/* 여기에 코드를 작성하세요 */

	/* 유저 공간이 아니면 오류 */
	if(is_kernel_vaddr(addr)) return false;
	
	void *upage = pg_round_down(addr);
	if(not_present){
		uintptr_t *rsp;
		if(user){
			rsp = f->rsp;
		}
		else{
			rsp = thread_current() -> tf.rsp;
		}
		void *fault_addr = addr;
		/* (uint8_t*)로 캐스팅해야하나?*/
		void *upage = pg_round_down(fault_addr);
		while(thread_current() -> stack_bottom > fault_addr && fault_addr >= rsp - 32){
			// printf("stack Growth함\n");
			if(addr <= USER_STACK - 0x100000){
				// printf("1MB limit over\n");
				return false;
			}
			void *new_page = thread_current() -> stack_bottom - PGSIZE;
			vm_stack_growth(new_page);
			thread_current() -> stack_bottom = thread_current() -> stack_bottom - PGSIZE;
			if(thread_current() -> stack_bottom < fault_addr){
				// printf("stack Growth완료 \n");
				return true;
			}
		}
		page = spt_find_page(spt, upage);
		if(page == NULL)
			return false;
		// printf("handler\n");
		// printf("Not Present : %d\n",not_present);
		if(write && !page->writable) return false;

		return vm_do_claim_page (page);
	}
	return false;
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
/* 페이지를 해제합니다.
 * 이 함수는 수정하지 마세요. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
/* VA에 할당된 페이지를 요청(클레임)합니다. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	struct thread *curr = thread_current();
	/* TODO: Fill this function */
	page = spt_find_page(&curr -> spt, va);
	if(page == NULL) return false;
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
/* PAGE를 요청(클레임)하고 MMU를 설정합니다. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();
	// printf("do_claim_page\n");
	/* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
	/* TODO: 페이지의 VA를 프레임의 PA에 매핑하기 위해 페이지 테이블 항목을 삽입하세요. */
	struct thread *curr = thread_current();
	if(!pml4_set_page(curr->pml4, page->va, frame->kva, page-> writable)){
		frame -> page = NULL; 
		page -> frame = NULL;
		palloc_free_page(frame -> kva);
		free(frame);
		return false;
	}
	return swap_in (page, frame->kva);
}

uint64_t spt_hash_func(const struct hash_elem *e, void *aux){
	struct page *pe = hash_entry(e, struct page, h_elem);
	 
	return hash_bytes(&pe->va, sizeof pe->va);
}

bool spt_hash_less_func(const struct hash_elem *a, 
	const struct hash_elem *b,
	void *aux){
		struct page *pa = hash_entry(a, struct page, h_elem);
		struct page *pb = hash_entry(b, struct page, h_elem);
		return pa->va < pb->va;
}

/* Initialize new supplemental page table */
/* 새로운 보조 페이지 테이블을 초기화합니다. */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
	hash_init(&spt->spt,spt_hash_func, spt_hash_less_func, NULL);
}

/* Copy supplemental page table from src to dst */
/* 보조 페이지 테이블을 src에서 dst로 복사합니다. */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
	struct hash_iterator src_itr = src->spt_iterator;
	hash_first(&src_itr, &src->spt);
	struct hash_elem *src_hash_elem;
	while(hash_next(&src_itr)){
		src_hash_elem = hash_cur(&src_itr);
		struct page *src_page = hash_entry(src_hash_elem, struct page, h_elem);

		enum vm_type src_type = src_page->operations->type;
		void *va = src_page->va;
		bool src_writable = src_page->writable;
		
		if(src_type == VM_UNINIT){
			vm_alloc_page_with_initializer(VM_FILE, va, src_writable, src_page->uninit.page_initializer, src_page->uninit.aux);
			continue;
		}
		// else if(src_type == VM_FILE){
		// 	struct container *file_aux = malloc(sizeof(struct container));
		// 	struct container *container = (struct container*)src_page->uninit.aux;
		// 	file_aux -> file = container->file;
		// 	file_aux -> ofs = container->ofs;
		// 	file_aux -> page_read_bytes = container ->page_read_bytes;
		// 	file_aux -> page_zero_bytes = container ->page_zero_bytes;
		// 	if(!vm_alloc_page_with_initializer(src_type, va, src_writable, NULL, file_aux)){
		// 		return false;
		// 	}
		// 	 struct page *file_page = spt_find_page(dst, va);
		// 	 file_backed_initializer(file_page, src_type, NULL);
		// 	 file_page->frame = src_page->frame;
		// 	 pml4_set_page(thread_current()->pml4, file_page->va, src_page->frame->kva, src_page->writable);
		// 	continue;
		// } 
		else{
			/* 가상 메모리 페이지 할당 */
			if(!vm_alloc_page(src_type, va, src_writable)){
				return false;
			}
			/* 할당된 페이지 요청 */
			if(!vm_claim_page(va)){
				return false;
			}
			// printf("kva : %p\n", src_page->frame->kva);
		}
		
		struct page *dst_page = spt_find_page(dst, va);

		memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
	}
	return true;
}

void hash_page_delete(struct hash_elem *e, void *aux){
	struct page *pg = hash_entry(e, struct page, h_elem);
	vm_dealloc_page(pg);
}

/* Free the resource hold by the supplemental page table */
/* 보조 페이지 테이블이 보유한 자원을 해제합니다. */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
		/* TODO: 쓰레드가 보유한 모든 보조 페이지 테이블을 파괴하고,
	 * TODO: 수정된 모든 내용을 저장소에 다시 기록(write-back)하세요. */
	hash_clear(&spt->spt, hash_page_delete);
}

