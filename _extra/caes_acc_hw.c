/**********************************************************/
/* This file is published under the GPLv2 license.        */
/* Copyright 2015 CAES-LAB_TEI-CRETE                      */
/* Check the license file.				  */
/**********************************************************/
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/dmaengine.h>
#include <linux/dma-mapping.h>  // dma access
#include <linux/seq_file.h> /* Needed for Sequence File Operations */
#include <linux/platform_device.h> /* Needed for Platform Driver Functions */
#include <linux/delay.h>

#include "caes_acc_hw.h"

#include <linux/ioctl.h>

#include <asm/dma-mapping.h>  // dma access
#include <asm/uaccess.h>  // dma access
#include <asm/uaccess.h> 
#include <asm/io.h> 
#include <asm/page.h>

/* Define Driver Name */
#define DRIVER_NAME "caes_acc_hw"

#define MAX_LOCK_TRIES 10

static int Major;
static int mem_minor = 0;
static int mem_nr_devs = 1;

struct mem_dev *mem_device; /* Allocated in acc_hw_probe */

static int acc_hw_open(struct inode *inode, struct file *file)
{
	struct mem_dev *dev;

	dev = container_of(inode->i_cdev, struct mem_dev, cdev);
	file->private_data = dev;
	return 0;
}

static int acc_hw_close(struct inode *inode, struct file *file)
{
	return 0;
}


/****************************************************************************
 *
 * Flush a Data cache line. If the byte specified by the address (adr)
 * is cached by the Data cache, the cacheline containing that byte is
 * invalidated.	If the cacheline is modified (dirty), the entire
 * contents of the cacheline are written to system memory before the
 * line is invalidated.
 *
 * @param	Address to be flushed.
 *
 * @return	None.
 *
 * @note		The bottom 6 bits are set to 0, forced by architecture.
 *
 ****************************************************************************/
void inline ARMv8_DCacheFlushLine(intptr_t  adr, cache_flush_t cm_type)
{
	uint32_t currmask;
	currmask = mfcpsr();
	mtcpsr(currmask | IRQ_FIQ_MASK);
	/* Select cache level 0 and D cache in CSSR */
	mtcp(CSSELR_EL1,0x0);
	if (cm_type==CVAC_T) {
		mtcpdc(CVAC,(adr & (~0x3F))); 
	} else if (cm_type==CIVAC_T) {
		mtcpdc(CIVAC,(adr & (~0x3F))); //CIVAC Clean and Invalidate by Virtual Address to Point of Coherency
	} else if (cm_type==CVAU_T) {
		mtcpdc(CVAU,(adr & (~0x3F))); 
	} else if (cm_type==CIVACU_T) {
		mtcpdc(CVAU,(adr & (~0x3F))); 
		dsb(sy);
		mtcpdc(IVAC,(adr & (~0x3F))); 
	} else if (cm_type==IVAC_T) {
		mtcpdc(IVAC,(adr & (~0x3F)));
	}	
	/* Wait for flush to complete */
	dsb(sy);
	/* Select cache level 1 and D cache in CSSR */
	mtcp(CSSELR_EL1,0x2);
	if (cm_type==CVAC_T) {
		mtcpdc(CVAC,(adr & (~0x3F))); 
	} else if (cm_type==CIVAC_T) {
		mtcpdc(CIVAC,(adr & (~0x3F))); //CIVAC Clean and Invalidate by Virtual Address to Point of Coherency
	} else if (cm_type==CVAU_T) {
		mtcpdc(CVAU,(adr & (~0x3F))); 
	} else if (cm_type==CIVACU_T) {
		mtcpdc(CVAU,(adr & (~0x3F))); 
		dsb(sy);
		mtcpdc(IVAC,(adr & (~0x3F))); 
	} else if (cm_type==IVAC_T) {
		mtcpdc(IVAC,(adr & (~0x3F)));
	}	
	/* Wait for flush to complete */
	dsb(sy);
	mtcpsr(currmask);
}

/****************************************************************************
 * Flush the Data cache for the given address range.
 * If the bytes specified by the address (adr) are cached by the Data cache,
 * the cacheline containing that byte is invalidated. If the cacheline
 * is modified (dirty), the written to system memory first before the
 * before the line is invalidated.
 *
 * @param	Start address of range to be flushed.
 * @param	Length of range to be flushed in bytes.
 *
 * @return	None.
 *
 * @note		None.
 *
 ****************************************************************************/

void inline ARMv8_DCacheFlushRange(intptr_t  adr, intptr_t len, cache_flush_t cm_type)
{
	const uint32_t cacheline = 64U;
	intptr_t end;
	intptr_t tempadr = adr;
	intptr_t tempend;
	uint32_t currmask;
	currmask = mfcpsr();
	mtcpsr(currmask | IRQ_FIQ_MASK);
	if (len != 0x00000000U) {
		end = tempadr + len;
		tempend = end;
		if ((tempadr & (0x3F)) != 0) {
			tempadr &= ~(0x3F);
			ARMv8_DCacheFlushLine(tempadr, cm_type);
			tempadr += cacheline;
		}
		if ((tempend & (0x3F)) != 0) {
			tempend &= ~(0x3F);
			ARMv8_DCacheFlushLine(tempend, cm_type);
		}

		while (tempadr < tempend) {
			/* Select cache level 0 and D cache in CSSR */
			mtcp(CSSELR_EL1,0x0);
			/* Flush Data cache line */
			if (cm_type==CVAC_T) {
				mtcpdc(CVAC,(tempadr & (~0x3F))); 
			} else if (cm_type==CIVAC_T) {
				mtcpdc(CIVAC,(tempadr & (~0x3F))); //Clean and Invalidate by VA to Point of Coherency
			} else if (cm_type==CVAU_T) {
				mtcpdc(CVAU,(tempadr & (~0x3F))); 
			} else if (cm_type==CIVACU_T) {
				mtcpdc(CVAU,(tempadr & (~0x3F))); 
				dsb(sy);
				mtcpdc(IVAC,(tempadr & (~0x3F))); 
			} else if (cm_type==IVAC_T) {
				mtcpdc(IVAC,(tempadr & (~0x3F))); 
			}
			/* Wait for flush to complete */
			dsb(sy);
			/* Select cache level 1 and D cache in CSSR */
			mtcp(CSSELR_EL1,0x2);
			/* Flush Data cache line */
			if (cm_type==CVAC_T) {
				mtcpdc(CVAC,(tempadr & (~0x3F))); 
			} else if (cm_type==CIVAC_T) {
				mtcpdc(CIVAC,(tempadr & (~0x3F))); //Clean and Invalidate by VA to Point of Coherency
			} else if (cm_type==CVAU_T) {
				mtcpdc(CVAU,(tempadr & (~0x3F))); 
			} else if (cm_type==CIVACU_T) {
				mtcpdc(CVAU,(tempadr & (~0x3F))); 
				dsb(sy);
				mtcpdc(IVAC,(tempadr & (~0x3F))); 
			} else if (cm_type==IVAC_T) {
				mtcpdc(IVAC,(tempadr & (~0x3F))); 
			}
			/* Wait for flush to complete */
			dsb(sy);
			tempadr += cacheline;
		}
	}

	mtcpsr(currmask);
}

static long 
acc_hw_ioctl(struct file *file, 
		unsigned int command, 
		const uint64_t __user *userinput) 
{

	struct mem_dev *dev = file->private_data;
	int copy_status, i, retval=0;
	uint32_t read_value, index;
	ioctl_args  *user_args;
	ioctl_args  *user_args_local_tmp;
	jobs *job_local;
	uint64_t *tmp_copy;
	uint32_t *tmp;

	/* Try to acquire lock */
	down_write(&dev->sem_job);
	//if ( down_interruptible(&dev->sem_job) ) {
	//	pr_err("[ERROR_MESSAGE: IOCTL] Could not acquire semaphore.\n");
	//	return -3;
	//}

	/* critical region (semaphore acquired) ... */

	switch(command)
	{
		case COMMAND_WRITE_JOB:

			/* Try to acquire lock */
			//if ( down_interruptible(&dev->sem_job) ) {
			//	pr_info("[ERROR_MESSAGE: IOCTL] Could not lock mutex.\n");
			//	return -3;
			//}

			/* critical region (semaphore acquired) ... */

			/*Check for available buffer in BRAM*/
			for (i=0; i<JOBS_BUFFER; i++) {
				read_value = ioread32(((uint32_t *)((uint64_t)dev->base_addr+JS_STATE_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(i*sizeof(jobs)))));
				//pr_info ("[INFO_MESSAGE: IOCTL] Read Value: %d. \n", read_value);
				if (read_value == ACCELERATOR_AVAILABLE) {
					index = i;
					//pr_info ("[INFO_MESSAGE: IOCTL] Read_value: %d, Buffer acquired: %d. \n", read_value, index);
					break;
				}
			}
			/*If no free buffer is found, return -1 and exit*/
			if ( i!=JOBS_BUFFER ) {
				retval = index;
			} else {
				retval = -1;
				pr_info ("[INFO_MESSAGE: IOCTL] No Buffer acquired: %d. \n", i);
				//hold off semaphore.	
				//up(&dev->sem_job);
				break;
			}

			iowrite32(ACCELERATOR_OCCUPIED, (uint32_t *)((uint64_t)dev->base_addr+JS_STATE_OFFSET+ BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs))));	
			//read_value = ioread32(((uint32_t *)((uint64_t)dev->base_addr+JS_STATE_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)))));
			//pr_info ("[INFO_MESSAGE: IOCTL] Writting accelerator accupied. Read_value: %d. \n", read_value);
			//mb(); //Memory barrier	
			//hold off semaphore.
			//up(&dev->sem_job);

			/*allocate static local variables*/
			jobInfo[index].user_args_local= (ioctl_args *) kmalloc(sizeof(ioctl_args), GFP_KERNEL);
			job_local      = (jobs *) kmalloc(sizeof(jobs), GFP_KERNEL);
			tmp_copy       = (uint64_t *)  kmalloc(sizeof(uint64_t), GFP_KERNEL);

			//fetch  users ioctl_args address
			copy_status = copy_from_user(tmp_copy, userinput, sizeof(uint64_t));	
			if (copy_status != 0) {
				pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Job struct) Failed (Status: %d)\n", copy_status);
				retval = -1;
				goto EXIT0;
			}

			//fetch user arguments 
			user_args = (ioctl_args *)*tmp_copy; 
			copy_status = copy_from_user(jobInfo[index].user_args_local, user_args, sizeof(ioctl_args));	
			if (copy_status != 0) {
				pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Job struct) Failed (Status: %d)\n", copy_status);
				retval = -1;
				goto EXIT0;
			} 
			//fetch job struct
			copy_status = copy_from_user(job_local, jobInfo[index].user_args_local->job, sizeof(jobs));	
			if (copy_status != 0) {
				pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Job struct) Failed (Status: %d)\n", copy_status);
				retval = -1;
				goto EXIT0;
			}

			//store application type from users copied job struct
			jobInfo[index].app_type = (uint8_t)job_local->arg0;

			if (jobInfo[index].app_type == OPERATION_HW_MATRIX_MULT) {
				//store matrices dimensions from users copied job struct
				jobInfo[index].matAcols = (uint8_t)job_local->res0;
				jobInfo[index].matArows = (uint8_t)(job_local->res0 >> 8);
				jobInfo[index].matBcols = job_local->res1;

				/* allocate consistent memory for DMA*/
				jobInfo[index].source_data_localA = dma_alloc_coherent (dev->device, jobInfo[index].matArows * jobInfo[index].matAcols * sizeof(int), &jobInfo[index].dmaphyssrcA, GFP_DMA | GFP_KERNEL);
				if (!jobInfo[index].source_data_localA) {
					pr_err ("[ERROR_MESSAGE: IOCTL] dma_alloc_coherent failed for sourceA data\n");
					retval = -1;
					goto EXIT0;
				}
				jobInfo[index].source_data_localB = dma_alloc_coherent (dev->device, jobInfo[index].matAcols * jobInfo[index].matBcols * sizeof(int), &jobInfo[index].dmaphyssrcB, GFP_DMA | GFP_KERNEL);
				if (!jobInfo[index].source_data_localB) {
					pr_err ("[ERROR_MESSAGE: IOCTL] dma_alloc_coherent failed for sourceB data\n");
					retval = -1;
					goto EXIT0;
				}
				jobInfo[index].dest_data_local = dma_alloc_coherent (dev->device, jobInfo[index].matArows * jobInfo[index].matBcols * sizeof(int), &jobInfo[index].dmaphysdst, GFP_DMA | GFP_KERNEL);
				if (!jobInfo[index].dest_data_local) {
					pr_err ("[ERROR_MESSAGE: IOCTL] dma_alloc_coherent failed for destination data\n");
					retval = -1;
					goto EXIT0;
				}


				//pr_info ("[INFO_MESSAGE: IOCTL] BufferA acquired: 0x%016llx, 0x%016llx.\n", jobInfo[index].source_data_localA, jobInfo[index].dmaphyssrcA);
				//pr_info ("[INFO_MESSAGE: IOCTL] BufferB acquired: 0x%016llx, 0x%016llx.\n", jobInfo[index].source_data_localB, jobInfo[index].dmaphyssrcB);
				//pr_info ("[INFO_MESSAGE: IOCTL] BufferC acquired: 0x%016llx, 0x%016llx.\n", jobInfo[index].dest_data_local, jobInfo[index].dmaphysdst);
				//fetch first matrix
				copy_status = copy_from_user(jobInfo[index].source_data_localA, jobInfo[index].user_args_local->sourceA, jobInfo[index].matArows*jobInfo[index].matAcols*sizeof(int));

				if (copy_status != 0) {
					pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Source DataA) Failed (Status: %d)\n", copy_status);
					retval = -1;
					goto EXIT0;
				}

				//fetch second matrix
				copy_status = copy_from_user(jobInfo[index].source_data_localB, jobInfo[index].user_args_local->sourceB, jobInfo[index].matAcols*jobInfo[index].matBcols*sizeof(int));
				if (copy_status != 0) {
					pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Source DataB) Failed (Status: %d)\n", copy_status);
					retval = -1;
					goto EXIT0;
				}

				//ARMv8_DCacheFlushRange((intptr_t)jobInfo[index].source_data_localA, jobInfo[index].matArows * jobInfo[index].matAcols * sizeof(int), CVAC_T);
				//ARMv8_DCacheFlushRange((intptr_t)jobInfo[index].source_data_localB, jobInfo[index].matAcols * jobInfo[index].matBcols * sizeof(int), CVAC_T);
				//ARMv8_DCacheFlushRange((intptr_t)jobInfo[index].dest_data_local, jobInfo[index].matArows * jobInfo[index].matBcols * sizeof(int), CIVAC_T);

				//write job to BRAM	  
				job_local->state    = STATUS_IDLE;
				job_local->start    = 0x0;
				job_local->status   = ACCELERATOR_OCCUPIED;
				job_local->type     = 0x0;
				job_local->arg1     = (unsigned long long)jobInfo[index].dmaphyssrcA;
				job_local->arg4     = (unsigned long long)jobInfo[index].dmaphyssrcB;
				job_local->arg2     = (uint16_t)jobInfo[index].matArows;
				job_local->arg2     <<=  sizeof(uint16_t)*4;
				job_local->arg2     |= (uint16_t)jobInfo[index].matAcols;
				job_local->arg3     = jobInfo[index].matBcols;
				job_local->res0     = 0x0;
				job_local->res1     = 0x0;
				job_local->result   = (unsigned long long)jobInfo[index].dmaphysdst;

			}else if (jobInfo[index].app_type == OPERATION_HW_SOBEL_ED) {
				//store image dimensions from users copieddd job struct
				jobInfo[index].im_width = job_local->res0;
				jobInfo[index].im_height = job_local->res1;

				//pr_info("App: %d\n", jobInfo[index].app_type);
				//pr_info("Width: %d\n", jobInfo[index].im_width);
				//pr_info("Height: %d\n", jobInfo[index].im_height);
				//pr_info("Alloc size: %dKB\n", jobInfo[index].im_height * jobInfo[index].im_width * 4 / 1024);

				/* allocate consistent memory for DMA*/
				jobInfo[index].source_data_localA = dma_alloc_coherent (dev->device, jobInfo[index].im_width * jobInfo[index].im_height * 4, &jobInfo[index].dmaphyssrcA, GFP_DMA | GFP_KERNEL);
				if (!jobInfo[index].source_data_localA) {
					pr_err ("[ERROR_MESSAGE: IOCTL] dma_alloc_coherent failed for sourceA data\n");
					retval = -1;
					goto EXIT0;
				}

				jobInfo[index].dest_data_local = dma_alloc_coherent (dev->device, jobInfo[index].im_width * jobInfo[index].im_height * 4, &jobInfo[index].dmaphysdst,  GFP_DMA | GFP_KERNEL);
				if (!jobInfo[index].dest_data_local) {
					pr_err ("[ERROR_MESSAGE: IOCTL] dma_alloc_coherent failed for destination data\n");
					retval = -1;
					goto EXIT0;
				}

				//pr_info ("[INFO_MESSAGE: IOCTL] Acquired buffer: %d.\n", index);
				//pr_info ("[INFO_MESSAGE: IOCTL] dest_data_local: 0x%016llX.\n", jobInfo[index].dest_data_local);
				//pr_info ("[INFO_MESSAGE: IOCTL] dmaphysdst: 0x%016llX.\n", jobInfo[index].dmaphysdst);
				//pr_info ("[INFO_MESSAGE: IOCTL] source_data_localA: 0x%016llX.\n", jobInfo[index].source_data_localA);
				//pr_info ("[INFO_MESSAGE: IOCTL] dmaphyssrcA: 0x%016llX.\n", jobInfo[index].dmaphyssrcA);
				//pr_info("IH: 0x%llX\n", (uint64_t)jobInfo[index].dmaphyssrcA);
				//pr_info("SOURCES: 0x%llX\n", (uint64_t)jobInfo[index].dmaphyssrcA);
				//pr_info("DESTINATION: 0x%llX\n", (uint64_t)jobInfo[index].dmaphysdst);

				//fetch image
				copy_status = copy_from_user(jobInfo[index].source_data_localA, jobInfo[index].user_args_local->sourceB, jobInfo[index].im_width * jobInfo[index].im_height * 4);

				if (copy_status != 0) {
					pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Source DataB) Failed (Status: %d)\n", copy_status);
					retval = -1;
					goto EXIT0;
				}
				//ARMv8_DCacheFlushRange((intptr_t)jobInfo[index].source_data_localA, jobInfo[index].matArows * jobInfo[index].matAcols * sizeof(int), CVAC_T);
				//ARMv8_DCacheFlushRange((intptr_t)jobInfo[index].source_data_localB, jobInfo[index].matAcols * jobInfo[index].matBcols * sizeof(int), CVAC_T);
				//ARMv8_DCacheFlushRange((intptr_t)jobInfo[index].dest_data_local, jobInfo[index].matArows * jobInfo[index].matBcols * sizeof(int), CIVAC_T);

				//write job to BRAM	  
				job_local->state    = STATUS_IDLE;
				job_local->start    = 0x0;
				job_local->status   = ACCELERATOR_OCCUPIED;
				job_local->type     = 0x0;
				job_local->arg1     = (unsigned long long)jobInfo[index].dmaphyssrcA;
				job_local->arg2     = jobInfo[index].im_width;
				job_local->arg3     = jobInfo[index].im_height;
				job_local->res0     = 0x0;
				job_local->res1     = 0x0;
				job_local->result   = (unsigned long long)jobInfo[index].dmaphysdst;
			}

			tmp = (uint32_t *) job_local;	   
			for (i=0; i<(sizeof(jobs)/4); i++) { //divide by 4 since iowrite32 writes 4 bytes
				iowrite32(tmp[i], (uint32_t *)((uint64_t)dev->base_addr+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs))+i*4));
			}

			//pr_info ("[INFO_MESSAGE: IOCTL] Write job OK. \n");
			//pr_info ("[INFO_MESSAGE: IOCTL] Write job OK, buffer %d. \n", index);
			//read_value = ioread32(((uint32_t *)((uint64_t)dev->base_addr+JS_STATE_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)))));
			//pr_info ("[INFO_MESSAGE: IOCTL] STATE[%d] = 0x%X. \n", index, read_value);
			//read_value = ioread32(((uint32_t *)((uint64_t)dev->base_addr+JS_START_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)))));
			//pr_info ("[INFO_MESSAGE: IOCTL] START[0x%X] = 0x%X. \n", JS_START_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)), read_value);
			//read_value = ioread32(((uint32_t *)((uint64_t)dev->base_addr+JS_STATUS_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)))));
			//pr_info ("[INFO_MESSAGE: IOCTL] STATUS[%d] = 0x%X. \n", index, read_value);
			
			kfree(tmp_copy);
			kfree(job_local);
			break;

EXIT0:
			iowrite32(ACCELERATOR_AVAILABLE, (uint32_t *)((uint64_t)dev->base_addr+JS_STATE_OFFSET+ BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs))));	
			kfree(tmp_copy);
			kfree(job_local);
			break;

		case COMMAND_START_JOB:

			tmp_copy = (uint64_t *) kmalloc(sizeof(uint64_t), GFP_KERNEL);
			user_args_local_tmp = (ioctl_args *) kmalloc(sizeof(ioctl_args), GFP_KERNEL);
			//fetch  users ioctl_args address
			copy_status = copy_from_user(tmp_copy, userinput, sizeof(uint64_t));	
			if (copy_status != 0) {
				pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Job struct) Failed (Status: %d)\n", copy_status);
				retval = -1;
				goto EXIT1;
			}
			//fetch user arguments
			user_args = (ioctl_args *)*tmp_copy; 
			copy_status = copy_from_user(user_args_local_tmp, user_args, sizeof(ioctl_args));	
			if (copy_status != 0) {
				pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Job struct) Failed (Status: %d)\n", copy_status);
				retval = -1;
				goto EXIT1;
			}
			//fetch buffer index from userland and store it to index variable
			copy_status = copy_from_user(&index, user_args_local_tmp->bufferIndex,sizeof(uint64_t));
			if (copy_status != 0) {
				pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Source Data) Failed (Status: %d)\n", copy_status);
				retval = -1;
				goto EXIT1;
			}

			iowrite32(0x1, (uint32_t *)((uint64_t)dev->base_addr+JS_START_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs))));
			//pr_info ("[INFO_MESSAGE: IOCTL] Buffer acquired: %d. \n", index);
			//pr_info ("[INFO_MESSAGE: IOCTL] Start job OK. \n");
EXIT1:
			kfree(tmp_copy);	  
			kfree(user_args_local_tmp);	

			//pr_info ("[INFO_MESSAGE: IOCTL] Start job OK, buffer %d. \n", index);
			break;

		case COMMAND_DONE_JOB:

			tmp_copy = (uint64_t *)  kmalloc(sizeof(uint64_t), GFP_KERNEL);
			user_args_local_tmp = (ioctl_args *)  kmalloc(sizeof(ioctl_args), GFP_KERNEL);
			//fetch  users ioctl_args address
			copy_status = copy_from_user(tmp_copy, userinput, sizeof(uint64_t));	
			if (copy_status != 0) {
				pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Job struct) Failed (Status: %d)\n", copy_status);
				retval = -1;
				goto EXIT2;
			}
			//fetch user arguments
			user_args = (ioctl_args *)*tmp_copy; 
			copy_status = copy_from_user(user_args_local_tmp, user_args, sizeof(ioctl_args));	
			if (copy_status != 0) {
				pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Job struct) Failed (Status: %d)\n", copy_status);
				retval = -1;
				goto EXIT2;
			}
			//fetch buffer index from userland and store it to index variable
			copy_status = copy_from_user(&index, user_args_local_tmp->bufferIndex,sizeof(uint64_t));
			if (copy_status != 0) {
				pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Source Data) Failed (Status: %d)\n", copy_status);
				retval = -1;
				goto EXIT2;
			}

			read_value = ioread32((uint32_t *)((uint64_t)dev->base_addr+JS_STATUS_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs))));
			if (read_value == STATUS_DONE) {
				retval = 0;	
			} 
			else if (read_value == STATUS_IDLE) {
				retval = -1;
				goto EXIT2;
			} 
			else if (read_value == STATUS_ERROR_UNKNOWN_FUNCTION) {
				retval = -2;
				pr_err ("[INFO_MESSAGE: IOCTL] Job unknown function. \n");
				goto EXIT2;
			}

			if (jobInfo[index].app_type == OPERATION_HW_MATRIX_MULT) {
				copy_status = copy_to_user(jobInfo[index].user_args_local->dest, jobInfo[index].dest_data_local, (jobInfo[index].matArows*jobInfo[index].matBcols*sizeof(int)));
			} else if (jobInfo[index].app_type == OPERATION_HW_SOBEL_ED) {

				//pr_info("App: %d\n", jobInfo[index].app_type);
				//pr_info("Width: %d\n", jobInfo[index].im_width);
				//pr_info("Height: %d\n", jobInfo[index].im_height);
				//pr_info("Alloc size: %dKB\n", jobInfo[index].im_height * jobInfo[index].im_width * 4 / 1024);
				//pr_info("Copy to: 0x%016llX\n", jobInfo[index].user_args_local->dest);

				copy_status = copy_to_user(jobInfo[index].user_args_local->dest, jobInfo[index].dest_data_local, (jobInfo[index].im_width*jobInfo[index].im_height*sizeof(int)));
				//copy_status = copy_to_user(jobInfo[index].user_args_local->dest, jobInfo[index].source_data_localA, (jobInfo[index].im_width*jobInfo[index].im_height*sizeof(int)));
			}

			if (copy_status != 0)
				pr_err ("[ERROR_MESSAGE: IOCTL] Copy To User (Destination Data) from 0x%X Failed (Status: %d)\n", (uint32_t)(uint64_t)jobInfo[index].dest_data_local, copy_status);

			//mb(); //Memory barrier
			//pr_info ("[INFO_MESSAGE: IOCTL] Job done, buffer %d. \n", index);			
			//read_value = ioread32(((uint32_t *)((uint64_t)dev->base_addr+JS_STATE_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)))));
			//pr_info ("[INFO_MESSAGE: IOCTL] STATE[%d] = 0x%X. \n", index, read_value);
			//read_value = ioread32(((uint32_t *)((uint64_t)dev->base_addr+JS_START_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)))));
			//pr_info ("[INFO_MESSAGE: IOCTL] START[0x%X] = 0x%X. \n", JS_START_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)), read_value);
			//read_value = ioread32(((uint32_t *)((uint64_t)dev->base_addr+JS_STATUS_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)))));
			//pr_info ("[INFO_MESSAGE: IOCTL] STATUS[%d] = 0x%X. \n", index, read_value);

			/* Reset BRAM job buffer  */
			//iowrite32(STATUS_IDLE, (uint32_t *)((uint64_t)dev->base_addr+JS_STATUS_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs))));	
			//iowrite32(ACCELERATOR_AVAILABLE, (uint32_t *)((uint64_t)dev->base_addr+JS_STATE_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs))));	  
EXIT2:
			//release allocated memory
			kfree(tmp_copy);	  
			kfree(user_args_local_tmp);	  

			break;

		case COMMAND_FREE_JOB:

			tmp_copy = (uint64_t *)  kmalloc(sizeof(uint64_t), GFP_KERNEL);
			user_args_local_tmp = (ioctl_args *)  kmalloc(sizeof(ioctl_args), GFP_KERNEL);
			//fetch  users ioctl_args address
			copy_status = copy_from_user(tmp_copy, userinput, sizeof(uint64_t));	
			if (copy_status != 0) {
				pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Job struct) Failed (Status: %d)\n", copy_status);
				retval = -1;
				goto EXIT3;
			}
			//fetch user arguments
			user_args = (ioctl_args *)*tmp_copy; 
			copy_status = copy_from_user(user_args_local_tmp, user_args, sizeof(ioctl_args));	
			if (copy_status != 0) {
				pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Job struct) Failed (Status: %d)\n", copy_status);
				retval = -1;
				goto EXIT3;
			}
			//fetch buffer index from userland and store it to index variable
			copy_status = copy_from_user(&index, user_args_local_tmp->bufferIndex,sizeof(uint64_t));
			if (copy_status != 0) {
				pr_err("[ERROR_MESSAGE: IOCTL] Copy From User (Source Data) Failed (Status: %d)\n", copy_status);
				retval = -1;
				goto EXIT3;
			}


			/* Try to acquire lock */
			//if ( down_interruptible(&dev->sem_job) ) {
			//	pr_info("[ERROR_MESSAGE: IOCTL] Could not lock mutex.\n");
			//	retval = -3;
			//	goto EXIT3;
			//}

			/* critical region (semaphore acquired) ... */
			if (jobInfo[index].app_type == OPERATION_HW_MATRIX_MULT) {
				dma_free_coherent (dev->device, jobInfo[index].matArows * jobInfo[index].matBcols * sizeof(int), jobInfo[index].dest_data_local, jobInfo[index].dmaphysdst);
				dma_free_coherent (dev->device, jobInfo[index].matArows * jobInfo[index].matAcols * sizeof(int), jobInfo[index].source_data_localA, jobInfo[index].dmaphyssrcA);
				dma_free_coherent (dev->device, jobInfo[index].matAcols * jobInfo[index].matBcols * sizeof(int), jobInfo[index].source_data_localB, jobInfo[index].dmaphyssrcB);
				//pr_info ("[INFO_MESSAGE: IOCTL] BufferA freed: 0x%016llx, 0x%016llx.\n", jobInfo[index].source_data_localA, jobInfo[index].dmaphyssrcA);
				//pr_info ("[INFO_MESSAGE: IOCTL] BufferB freed: 0x%016llx, 0x%016llx.\n", jobInfo[index].source_data_localB, jobInfo[index].dmaphyssrcB);
				//pr_info ("[INFO_MESSAGE: IOCTL] BufferC freed: 0x%016llx, 0x%016llx.\n", jobInfo[index].dest_data_local, jobInfo[index].dmaphysdst);

			} else if (jobInfo[index].app_type == OPERATION_HW_SOBEL_ED) {
				//pr_info ("[INFO_MESSAGE: IOCTL] Job free, buffer %d. \n", index);
				//pr_info ("[INFO_MESSAGE: IOCTL] im_width: , im_height: %d.\n", jobInfo[index].im_width, jobInfo[index].im_height);
				//pr_info ("[INFO_MESSAGE: IOCTL] dest_data_local: 0x%016llX.\n", jobInfo[index].dest_data_local);
				//pr_info ("[INFO_MESSAGE: IOCTL] dmaphysdst: 0x%016llX.\n", jobInfo[index].dmaphysdst);
				//pr_info ("[INFO_MESSAGE: IOCTL] source_data_localA: 0x%016llX.\n", jobInfo[index].source_data_localA);
				//pr_info ("[INFO_MESSAGE: IOCTL] dmaphyssrcA: 0x%016llX.\n", jobInfo[index].dmaphyssrcA);
				dma_free_coherent (dev->device, jobInfo[index].im_width * jobInfo[index].im_height * 4, jobInfo[index].dest_data_local, jobInfo[index].dmaphysdst);
				dma_free_coherent (dev->device, jobInfo[index].im_width * jobInfo[index].im_height * 4, jobInfo[index].source_data_localA, jobInfo[index].dmaphyssrcA);
			}

			
			/* Reset BRAM job buffer  */
			iowrite32(STATUS_IDLE, (uint32_t *)((uint64_t)dev->base_addr+JS_STATUS_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs))));	
			iowrite32(ACCELERATOR_AVAILABLE, (uint32_t *)((uint64_t)dev->base_addr+JS_STATE_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs))));	  
			
			//hold off semaphore.
			//up(&dev->sem_job);
			//pr_info ("[INFO_MESSAGE: IOCTL] Job freed, buffer %d. \n", index);
			//read_value = ioread32(((uint32_t *)((uint64_t)dev->base_addr+JS_STATE_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)))));
			//pr_info ("[INFO_MESSAGE: IOCTL] STATE[%d] = 0x%X. \n", index, read_value);
			//read_value = ioread32(((uint32_t *)((uint64_t)dev->base_addr+JS_START_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)))));
			//pr_info ("[INFO_MESSAGE: IOCTL] START[0x%X] = 0x%X. \n", JS_START_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)), read_value);
			//read_value = ioread32(((uint32_t *)((uint64_t)dev->base_addr+JS_STATUS_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(index*sizeof(jobs)))));
			//pr_info ("[INFO_MESSAGE: IOCTL] STATUS[%d] = 0x%X. \n", index, read_value);

EXIT3:	
			//release allocated memory
			kfree(jobInfo[index].user_args_local);
			kfree(tmp_copy);	  
			kfree(user_args_local_tmp);	  

			break;

		case COMMAND_RESTART_CONTROLLER:

			/* Request subsystem restart */
			//iowrite32(0x1, (uint32_t *)((uint64_t)dev->base_addr+JS_RESTART_CONTROLLER));	
			//mb(); //read write memory barrier
			//do {
			//	read_value = ioread32((uint32_t *)((uint64_t)dev->base_addr+JS_RESTART_CONTROLLER));
			//	msleep(10);
			//} while(read_value == 0x1);
			pr_info("Command restart controller, not implemented in this version\n");
			break;

		default:
			pr_info ("[INFO_MESSAGE: IOCTL] Default reached. \n");
			break;		

	}

	mb(); //read write memory barrier
	/* finally, awake any threads and return */
	//wake_up_interruptible(&dev->acc_hw_q);
	//hold off semaphore.
	up_write(&dev->sem_job);

	//pr_info ("[INFO_MESSAGE: IOCTL] retval: %d. \n", retval);
	return retval;
}


/* File Operations for /dev/mem_ioX */

static const struct file_operations acc_hw_fops =
{
	.open = acc_hw_open,
	.release = acc_hw_close, 
	.unlocked_ioctl = (long)acc_hw_ioctl
};

/* Remove function for mem_io */

static int mem_remove(struct platform_device *pdev)
{
	int i;
	dev_t devno = MKDEV(Major, mem_minor);
	if (mem_device)
	{
		for(i = 0;i < mem_nr_devs; i++)
		{
			cdev_del(&mem_device[i].cdev);
		}
		kfree(mem_device);
	}
	
	/* Release allocated number */
	unregister_chrdev_region(devno, mem_nr_devs);
	/* Release mapped virtual address */
	iounmap(mem_device->base_addr);
	/* Release the region */
	release_mem_region(mem_device->res->start, mem_device->remap_size);
	return 0;
}

static void mem_setup_cdev(struct mem_dev *dev, int index)
{
	int ret, mem_dev_no = MKDEV(Major,mem_minor + index);

	/* register device structure */
	cdev_init(&mem_device->cdev, &acc_hw_fops);
	mem_device->cdev.owner = THIS_MODULE;
	mem_device->cdev.ops = &acc_hw_fops;
	ret = cdev_add(&mem_device->cdev, mem_dev_no, 1);
	if(ret < 0 )
		printk(KERN_NOTICE "Unable %d adding mem_io%d ", ret, index);
}

/* Device Probe function for mem_io */

static int mem_probe(struct platform_device *pdev) 
{
	dev_t mem_dev_no = 0;
	int ret, i = 0;

	ret = alloc_chrdev_region(&mem_dev_no, mem_minor, mem_nr_devs, DRIVER_NAME);
	if(ret < 0)
	{
		printk(KERN_ERR "Major number allocation failed\n");
		return ret;
	}

	Major = MAJOR(mem_dev_no);
	printk(KERN_INFO "The major number is %d",Major); 

	/* Allocate mem_dev struct */
	mem_device = kmalloc(mem_nr_devs * sizeof(struct mem_dev), GFP_KERNEL);
	if (!mem_device)
	{
		ret = -ENOMEM;
		goto fail;
	}
	for(i = 0;i < mem_nr_devs; i++)
	{
		/* Initialize wait queue */
		//init_waitqueue_head(&mem_device[i].acc_hw_q);
		/* Initialize semaphore, count is 1 */
		//sema_init(&mem_device[i].sem_job, 1);
		init_rwsem(&mem_device[i].sem_job);
		mem_setup_cdev(&mem_device[i], i);
	}

	mem_device->device = &pdev->dev;
	/* Now time to get the hardware */
	mem_device->res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!mem_device->res) 
	{
		dev_err(&pdev->dev, "No memory resource\n");
		return -ENODEV;
	}

	mem_device->remap_size = mem_device->res->end - mem_device->res->start + 1;
	if (!request_mem_region(mem_device->res->start, mem_device->remap_size, pdev->name)) 
	{
		dev_err(&pdev->dev, "Cannot request IO\n");
		return -ENXIO;
	}

	mem_device->base_addr = ioremap(mem_device->res->start, mem_device->remap_size);
	if (mem_device->base_addr == NULL) 
	{
		dev_err(&pdev->dev, "Couldn't ioremap memory at 0x%08lx\n",
				(unsigned long)mem_device->res->start);
		ret = -ENOMEM;
		goto err_release_region;
	}

	printk(KERN_INFO DRIVER_NAME " probed at VA 0x%08lx\n",
			(unsigned long) mem_device->base_addr);

	for(i=0; i<JOBS_BUFFER; i++) {
		//pr_info("jobs: %d\n", sizeof(jobs));
		iowrite32(ACCELERATOR_AVAILABLE, (uint32_t *)((uint64_t)mem_device->base_addr+JS_STATE_OFFSET +BRAM_JOBS_BASE_ADDR_GPPU+(i*sizeof(jobs))));	
		iowrite32(STATUS_IDLE          , (uint32_t *)((uint64_t)mem_device->base_addr+JS_STATUS_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(i*sizeof(jobs))));	
		iowrite32(0x0                  , (uint32_t *)((uint64_t)mem_device->base_addr+JS_START_OFFSET +BRAM_JOBS_BASE_ADDR_GPPU+(i*sizeof(jobs))));
		//iowrite32(0xA, (uint32_t *)mem_device->base_addr+JS_STATE_OFFSET +BRAM_JOBS_BASE_ADDR_GPPU+(i*sizeof(jobs)/4));	
		//iowrite32(0xB          , (uint32_t *)mem_device->base_addr+JS_STATUS_OFFSET+BRAM_JOBS_BASE_ADDR_GPPU+(i*sizeof(jobs)/4));	
		//iowrite32(0xC                  , (uint32_t *)mem_device->base_addr+JS_START_OFFSET +BRAM_JOBS_BASE_ADDR_GPPU+(i*sizeof(jobs)/4));
	}
	return 0;

fail:
err_release_region:
	mem_remove(pdev);
	return ret;
}

/* device match table to match with device node in device tree */

static const struct of_device_id acc_hw_of_match[] = 
{
	{.compatible = DRIVER_NAME},
	{},
};

MODULE_DEVICE_TABLE(of, acc_hw_of_match);

/* platform driver structure for mem_io driver */

static struct platform_driver acc_hw_driver = 
{
	.driver = {
		.name = DRIVER_NAME,
		.owner = THIS_MODULE,
		.of_match_table = acc_hw_of_match},
	.probe = mem_probe,
	.remove = mem_remove
};

/* Register mem_io platform driver */

module_platform_driver(acc_hw_driver);

/* Module Infomations */

MODULE_AUTHOR("CAES-LAB_TEI_CRETE");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(DRIVER_NAME ": CAES HW accelerators driver");
MODULE_ALIAS(DRIVER_NAME);
