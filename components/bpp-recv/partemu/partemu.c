/*
Quick and dirty file-backed implementation of the esp32 partition api with files as backing to
compile and test the flash blockdevice backends on a host cpu.
*/
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_partition.h"
#include <sys/mman.h>

const esp_partition_t *esp_partition_find_first(esp_partition_type_t type, esp_partition_subtype_t subtype, const char *label) {
	char buf[128];
	esp_partition_t *part=malloc(sizeof(esp_partition_t));
	sprintf(buf, "part-%d-%d.img", (int)type, (int)subtype);
	int f=open(buf, O_RDWR|O_CREAT, 0644);
	if (f<=0) {
		perror(buf);
		exit(1);
	}
	part->address=f;
	part->type=type;
	part->subtype=subtype;
	part->size=lseek(part->address, 0, SEEK_END);
	return part;
}

esp_err_t esp_partition_read(const esp_partition_t *part, size_t src_offset, void *dst, size_t size) {
	lseek((int)part->address, src_offset, SEEK_SET);
	read((int)part->address, dst, size);
	return ESP_OK;
}

esp_err_t esp_partition_write(const esp_partition_t *part, size_t dst_offset, const void *src, size_t size) {
	uint8_t *buf=malloc(size);
	uint8_t *bsrc=(uint8_t*)src;
	lseek((int)part->address, dst_offset, SEEK_SET);
	//Flash bits can only be cleared when writing. Simulate that here.
	read((int)part->address, buf, size);
	for (int i=0; i<size; i++) buf[i]&=bsrc[i];
	lseek((int)part->address, dst_offset, SEEK_SET);
	write((int)part->address, buf, size);
	return ESP_OK;
}

esp_err_t esp_partition_erase_range(const esp_partition_t *part, uint32_t start_addr, uint32_t size) {
	//must be on page border
	assert((start_addr&4095)==0);
	assert((size&4095)==0);
	//write all f's
	uint8_t *buf=malloc(size);
	for (int i=0; i<size; i++) buf[i]=0xff;
	lseek((int)part->address, start_addr, SEEK_SET);
	write((int)part->address, buf, size);
	return ESP_OK;
}

esp_err_t esp_partition_mmap(const esp_partition_t* partition, uint32_t offset, uint32_t size,
                             spi_flash_mmap_memory_t memory,
                             const void** out_ptr, spi_flash_mmap_handle_t* out_handle) {

	*out_ptr=mmap(NULL, size, PROT_READ, MAP_SHARED, partition->address, offset);
	assert (*out_ptr!=MAP_FAILED);
	return ESP_OK;
}


