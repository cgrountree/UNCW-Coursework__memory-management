#include <stdlib.h>
#include "memory.h"
#include "queueLibrary.h"


char *toString(FRAME *f)
{
    static char result[BUFSIZ];

    // For each from that is printed, print the frame number, pcb id of the
    // process that owns it, the page number of the page it contains,
    // D or C if it is dirty or clean, and the lock count.

    if (f == NULL) sprintf (result, "(null) ");
    else sprintf (result, "Frame %d(pcb-%d,page-%d,%c,lock-%d) ",
               f->frame_id,
               f->pcb->pcb_id,
               f->page_id,
               (f->dirty ? 'D' : 'C'),
               f->lock_count);

    return result;
}

Queue myQueue;
void memory_init()
{
        initQueue(&myQueue);
}

int compareTo(FRAME *frame1, FRAME *frame2)
{
        return frame1 -> frame_id - frame2 -> frame_id;
}


void reference(int logic_addr, REFER_ACTION action)
{
        int page_no;
        int offset;
        int phys_addr;
        PAGE_TBL *page_tbl_ptr;
        PCB *currentPCB;
        currentPCB = PTBR -> pcb;
        page_no = logic_addr / PAGE_SIZE;
        offset = logic_addr % PAGE_SIZE;
        page_tbl_ptr = currentPCB -> page_tbl;
        if(logic_addr < MAX_PAGE*PAGE_SIZE && logic_addr >= 0){
                if(page_tbl_ptr -> page_entry[page_no].valid == false) {
                        Int_Vector.pcb = currentPCB;
                        Int_Vector.page_id = page_no;
                        Int_Vector.cause = pagefault;
                        gen_int_handler();
                }

                if((page_tbl_ptr -> page_entry[page_no].valid == true) && (action == store)) {
                        Frame_Tbl[currentPCB -> page_tbl -> page_entry[page_no].frame_id].dirty = true;
                }
                removeNode(&myQueue, findNode(&myQueue, &Frame_Tbl[currentPCB -> page_tbl -> page_entry[page_no].
                                                                        frame_id], compareTo));
                enQueue(&myQueue, &Frame_Tbl[currentPCB -> page_tbl -> page_entry[page_no].frame_id]);
                phys_addr = (page_no * PAGE_SIZE) * offset;
                memoryAccess(action, currentPCB -> page_tbl -> page_entry[page_no].frame_id, offset);
        }
}


// unused
void prepage(PCB *pcb)
{
        return;

}

// unused
int start_cost(PCB *pcb)
{
        return 0;

}



void deallocate(PCB *pcb)
{
        if(&myQueue == NULL) {
                return;
        }
        for(int i = 0; i < MAX_PAGE; i++){
                PAGE_ENTRY *thePage = &(pcb -> page_tbl -> page_entry[i]);
                if( thePage -> valid == true) {
                        FRAME *theFrame = &Frame_Tbl[thePage -> frame_id];
                        theFrame -> pcb = NULL;
                        theFrame -> dirty = true;
                        theFrame -> page_id = 0;
                        removeNode(&myQueue, findNode(&myQueue, theFrame, compareTo));
                        theFrame -> free = true;
                }
        }

}



void get_page(PCB *pcb, int page_id)
{
        FRAME *victim = NULL;

        for(int i = 0; i < MAX_FRAME; i++){
                if (Frame_Tbl[i].free && Frame_Tbl[i].lock_count == 0) {
                        victim = &Frame_Tbl[i];
                        break;
                }
        }
        if(victim == NULL){
                QueueNode *current = frontNode(&myQueue);
                while(current != NULL) {
                        victim = current -> data;
                        if(victim -> lock_count == 0) {
                                break;
                        }
                        current = current -> next;
                }
                if (victim -> dirty == true) {
                        victim -> lock_count = 1;
                        siodrum(write, victim -> pcb, victim -> page_id, victim -> frame_id);
                        victim -> dirty = false;
                }
                PAGE_ENTRY *newEntry;
                newEntry = &victim -> pcb -> page_tbl -> page_entry[victim ->page_id];
                newEntry -> valid = false;
        }
        victim -> lock_count = 1;
        victim -> free = false;
        siodrum(read, pcb, page_id, victim -> frame_id);
        victim -> lock_count = 0;
        victim -> pcb = pcb;
        victim -> page_id = page_id;
        pcb -> page_tbl -> page_entry[page_id].frame_id = victim -> frame_id;
        victim -> dirty = false;
        pcb -> page_tbl -> page_entry[page_id].valid = true;
        removeNode(&myQueue, findNode(&myQueue, &Frame_Tbl[victim -> frame_id], compareTo));
        enQueueSorted(&myQueue, &Frame_Tbl[victim -> frame_id], compareTo);

}



void lock_page(IORB *iorb)
{
        int page_id;
        int frame_id;
        PAGE_TBL *page_tbl_ptr;

        page_tbl_ptr = iorb -> pcb -> page_tbl;
        page_id = iorb -> page_id;

        if(page_tbl_ptr -> page_entry[page_id].valid == false) {
                Int_Vector.pcb = iorb -> pcb;
                Int_Vector.page_id = page_id;
                Int_Vector.cause = pagefault;
                gen_int_handler();
        }
        frame_id = page_tbl_ptr -> page_entry[page_id].frame_id;

        if (iorb -> action == read)
                Frame_Tbl[frame_id].dirty = true;
        Frame_Tbl[frame_id].lock_count++;

}



void unlock_page(IORB  *iorb)
{
        int page_id;
        int frame_id;
        PAGE_TBL *page_tbl_ptr;
        page_tbl_ptr = iorb -> pcb -> page_tbl;
        page_id = iorb -> page_id;
        frame_id = page_tbl_ptr -> page_entry[page_id].frame_id;

        Frame_Tbl[frame_id].lock_count--;

}
