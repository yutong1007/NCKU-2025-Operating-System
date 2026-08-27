#include <linux/fs.h>
#include <linux/uaccess.h>
#include "osfs.h"

/**
 * Function: osfs_read
 * Description: Reads data from a file.
 * Inputs:
 *   - filp: The file pointer representing the file to read from.
 *   - buf: The user-space buffer to copy the data into.
 *   - len: The number of bytes to read.
 *   - ppos: The file position pointer.
 * Returns:
 *   - The number of bytes read on success.
 *   - 0 if the end of the file is reached.
 *   - -EFAULT if copying data to user space fails.
 */
static ssize_t osfs_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
    struct inode *inode = file_inode(filp);
    struct osfs_inode *osfs_inode = inode->i_private;
    struct osfs_sb_info *sb_info = inode->i_sb->s_fs_info;
    void *data_block;
    ssize_t bytes_read;

    // If the file has not been allocated a data block, it indicates the file is empty
    if (osfs_inode->i_blocks == 0)
        return 0;

    if (*ppos >= osfs_inode->i_size)
        return 0;

    if (*ppos + len > osfs_inode->i_size)
        len = osfs_inode->i_size - *ppos;

    data_block = sb_info->data_blocks + osfs_inode->i_block * BLOCK_SIZE + *ppos;
    if (copy_to_user(buf, data_block, len))
        return -EFAULT;

    *ppos += len;
    bytes_read = len;

    return bytes_read;
}


/**
 * Function: osfs_write
 * Description: Writes data to a file.
 * Inputs:
 *   - filp: The file pointer representing the file to write to.
 *   - buf: The user-space buffer containing the data to write.
 *   - len: The number of bytes to write.
 *   - ppos: The file position pointer.
 * Returns:
 *   - The number of bytes written on success.
 *   - -EFAULT if copying data from user space fails.
 *   - Adjusted length if the write exceeds the block size.
 */
static ssize_t osfs_write(struct file *filp, const char __user *buf, size_t len, loff_t *ppos)
{   
    /*
    1. get the open file inode
    2. Allocates a free data block from the block bitmap
    3. Copies data from user space to kernel space
    */
    // Step1: Retrieve the inode and filesystem information
    struct inode *inode = file_inode(filp);
    struct osfs_inode *osfs_inode = inode->i_private;
    struct osfs_sb_info *sb_info = inode->i_sb->s_fs_info;
    void *data_block;
    ssize_t bytes_written;
    int ret;

    // Step2: Check if a data block has been allocated; if not, allocate one
    // if need, alloc data block
    if (osfs_inode->i_blocks == 0 || osfs_inode->i_block == 0) {
        uint32_t new_block;
    
        ret = osfs_alloc_data_block(sb_info, &new_block);
        if (ret) {
            pr_err("osfs_write: Failed to allocate data block\n");
            return ret;
        }

        osfs_inode->i_block = new_block;
        osfs_inode->i_blocks = 1;

        memset(sb_info->data_blocks + new_block * BLOCK_SIZE, 0, BLOCK_SIZE);
    }

    // Step3: Limit the write length to fit within one data block
    // write in the data block
    if (*ppos >= BLOCK_SIZE)
        return 0;
    // at most one block content
    if (len > (size_t)(BLOCK_SIZE - *ppos))
        len = (size_t)(BLOCK_SIZE - *ppos);

    // Step4: Write data from user space to the data block
    // get memory location
    data_block = sb_info->data_blocks + osfs_inode->i_block * BLOCK_SIZE + *ppos;
    if (copy_from_user(data_block, buf, len))
        return -EFAULT;

    bytes_written = (ssize_t)len;

    // Step5: Update inode & osfs_inode attribute
    // offset (finish position)
    *ppos += bytes_written;

    if (*ppos > osfs_inode->i_size)
        osfs_inode->i_size = (uint32_t)(*ppos);

    inode->i_size = osfs_inode->i_size;
    {        
        struct timespec64 now = current_time(inode);
        inode_set_ctime_to_ts(inode, now);
        inode_set_mtime_to_ts(inode, now);
    }
    mark_inode_dirty(inode);

    // Step6: Return the number of bytes written    
    return bytes_written;
}

/**
 * Struct: osfs_file_operations
 * Description: Defines the file operations for regular files in osfs.
 */
const struct file_operations osfs_file_operations = {
    .open = generic_file_open, // Use generic open or implement osfs_open if needed
    .read = osfs_read,
    .write = osfs_write,
    .llseek = default_llseek,
    // Add other operations as needed
};

/**
 * Struct: osfs_file_inode_operations
 * Description: Defines the inode operations for regular files in osfs.
 * Note: Add additional operations such as getattr as needed.
 */
const struct inode_operations osfs_file_inode_operations = {
    // Add inode operations here, e.g., .getattr = osfs_getattr,
};
