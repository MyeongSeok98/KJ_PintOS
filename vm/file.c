/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "userprog/process.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "threads/mmu.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	// printf("[file_backed_initializer] start\n");
	page->operations = &file_ops;
	
	struct file_page *file_page = &page->file;  

	// struct container *container = (struct container *)page->uninit.aux;
	// file_page->file = container->file;
	// file_page->ofs = container->ofs;
	// file_page->read_bytes = container->page_read_bytes;
	// file_page->zero_bytes = container->page_zero_bytes;
	return true;
}

/* Initialize the file backed page */
bool
fork_file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	printf("[file_backed_initializer] start\n");
	page->operations = &file_ops;
	
	struct file_page *file_page = &page->file;  
	return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool 
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
	// printf("[file_backed_destroy] start \n");
	// printf("[file_backed_destroy] file : %d\n", file_page ->file);
	// printf("[file_backed_destroy] page_read_bytes : %d\n", file_page ->read_bytes);
	// printf("[file_backed_destroy] page_zero_bytes : %d\n", file_page ->zero_bytes);
	// printf("[file_backed_destroy] ofs : %d\n", file_page ->ofs);
	struct thread *t = thread_current();

	/* 테스트 확인용 */
	// if(pml4_is_dirty(t->pml4, page->va)){			
	// 	// printf("[file_backed_destroy] is_dirty - yes\n");
	// 	file_write_at(file_page->file, page->va, file_page->read_bytes, file_page->ofs);
	// 	pml4_set_dirty(t->pml4, page->va, 0);
	// }
	
	// pml4_clear_page(t->pml4, page->va);
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {
	struct thread *curr = thread_current(); 
	// struct page *pg = spt_find_page(&curr-> spt, addr);
	size_t calculate_length = file_length(file);
	void *backup_addr = addr;
	/* testing*/
	int count = 0;
	struct file *new_file = file_reopen(file);
	/* */
	while(calculate_length > 0){
		// printf("[mmap] calculate_length : %d\n", calculate_length);
		uint32_t page_read_bytes;
		if(calculate_length >= PGSIZE) page_read_bytes = PGSIZE; 
		else page_read_bytes = calculate_length;
		
		if(is_kernel_vaddr(addr+ PGSIZE)) return NULL;
		uint32_t page_zero_bytes = PGSIZE - page_read_bytes;
		struct container *aux = calloc(1, sizeof (struct container));
		aux -> file = new_file;
		aux -> ofs = offset;
		aux -> page_read_bytes = page_read_bytes;
		aux -> page_zero_bytes = page_zero_bytes;

		if(!vm_alloc_page_with_initializer(VM_FILE, addr, writable, lazy_load_segment, aux)){
			free(aux);
			return NULL;
		}
		struct page *page = spt_find_page(&curr-> spt, addr);
		page->modified = thread_current() -> modified_pages;

		calculate_length -= page_read_bytes;
		offset += page_read_bytes; 
		addr += PGSIZE;
		count++;
		// printf("[mmap] addr : %p\n", addr); 
		// printf("calculat_length : %d\n", calculate_length);
	}
	//printf("[mmap] count = %d\n", count);
	// printf("[mmap] backup_addr : %p\n", backup_addr); 
	return backup_addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
	struct supplemental_page_table *spt = &thread_current() -> spt;
	struct page *page = spt_find_page(spt, addr);
	//printf("[munmap] page_modified : %d\n", page->modified);

	int page_number = page->modified;
	struct container *aux = (struct container *)page->uninit.aux;
	// // //printf("[do_munmap] page_number : %d\n", page_number);
	while(page && page -> modified == page_number){
		// printf("[do_munmap] modified = %d\n",page -> modified);
		if(page){
			if(pml4_is_dirty(thread_current()->pml4, page->va)){
				file_write_at(aux->file, page->va, aux->page_read_bytes, aux->ofs);
				pml4_set_dirty(thread_current()->pml4, page->va, 0);
			}
			destroy(page);
			pml4_clear_page(thread_current()->pml4, addr); 
		}
		// struct container* container = (struct container*)page->uninit.aux;
		// page->file.aux = container;

		// file_backed_destroy(page);
		addr+= PGSIZE;
		page = spt_find_page(spt, addr);
		// printf("[do_munmap] page_number next : %d\n", page -> modified);
	}
}
// void
// do_munmap (void *addr) {
// 	struct page *page = NULL;

// 	int count = 0;
// 	while((page = spt_find_page(&thread_current()->spt, addr)) != NULL){
// 		struct container *aux = (struct container *)page->uninit.aux;
// 		if(pml4_is_dirty(thread_current()->pml4, page->va)){
// 				file_write_at(aux->file, page->va, aux->page_read_bytes, aux->ofs);
// 				pml4_set_dirty(thread_current()->pml4, page->va, 0);
// 		}
// 		count++;
// 		//spt_remove_page(thread_current()->spt, page);
// 		pml4_clear_page(thread_current()->pml4, addr); 
// 		addr += PGSIZE;
// 	}
// }