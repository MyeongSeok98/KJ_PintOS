/* uninit.c: Implementation of uninitialized page.
 *
 * All of the pages are born as uninit page. When the first page fault occurs,
 * the handler chain calls uninit_initialize (page->operations.swap_in).
 * The uninit_initialize function transmutes the page into the specific page
 * object (anon, file, page_cache), by initializing the page object,and calls
 * initialization callback that passed from vm_alloc_page_with_initializer
 * function.
 * */

/*
	모든 페이지는 초기화되지 않은 상태로 깨어납니다. 첫번째 페이지 폴트가 발생하면,
 * 	handler chain이 uninit_initialize(page->operations.swap_in)를 호출합니다.
 *	uninit_initialize 함수는 페이지 객체를 초기화하여 해당 페이지를 
 *  특정 페이지 객체(anon, file, page_cache)로 변환하고, vm_alloc_page_with_initializer
 *  함수에서 전달된 초기화 콜백을 호출합니다.
 */

#include "vm/vm.h"
#include "vm/uninit.h"

static bool uninit_initialize (struct page *page, void *kva);
static void uninit_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations uninit_ops = {
	.swap_in = uninit_initialize,
	.swap_out = NULL,
	.destroy = uninit_destroy,
	.type = VM_UNINIT,
};

/* DO NOT MODIFY this function */
void
uninit_new (struct page *page, void *va, vm_initializer *init,
		enum vm_type type, void *aux,
		bool (*initializer)(struct page *, enum vm_type, void *)) {
	ASSERT (page != NULL);

	*page = (struct page) {
		.operations = &uninit_ops,
		.va = va,
		.frame = NULL, /* no frame for now */
		.uninit = (struct uninit_page) {
			.init = init,
			.type = type,
			.aux = aux,
			.page_initializer = initializer,
		}
	};
}

/* Initalize the page on first fault */
/* 첫 페이지 폴트때문에 페이지 시작시킴 */
static bool
uninit_initialize (struct page *page, void *kva) {
	struct uninit_page *uninit = &page->uninit;

	/* Fetch first, page_initialize may overwrite the values */
	 /* 먼저 값을 가져옵니다. page_initialize가 아래 값을 덮어쓸 수 있기 때문입니다. */
	vm_initializer *init = uninit->init;
	void *aux = uninit->aux;

	/* TODO: You may need to fix this function. */

	/* 페이지 종류에 따라 다른 page_initializer를 이용해 초기화 수행
	 * 이호출이 true면 뒤의 호출도 수행한다. 
	 * init이 NULL 이 아니면 init(page, aux)를 수행한다. 필요없으면 true를 호출한다.*/
	return uninit->page_initializer (page, uninit->type, kva) &&
		(init ? init (page, aux) : true);
}

/* Free the resources hold by uninit_page. Although most of pages are transmuted
 * to other page objects, it is possible to have uninit pages when the process
 * exit, which are never referenced during the execution.
 * PAGE will be freed by the caller. */
static void
uninit_destroy (struct page *page) {
	struct uninit_page *uninit UNUSED = &page->uninit;
	/* TODO: Fill this function.
	 * TODO: If you don't have anything to do, just return. */
}
