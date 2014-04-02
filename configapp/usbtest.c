/**
 * Project: AVR ATtiny USB Tutorial at http://codeandlife.com/
 * Author: Joonas Pihlajamaa, joonas.pihlajamaa@iki.fi
 * Based on V-USB example code by Christian Starkjohann
 * Copyright: (C) 2012 by Joonas Pihlajamaa
 * License: GNU GPL v3 (see License.txt)
 *
 * (Now heavily modified.)
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// this is libusb, see http://libusb.sourceforge.net/ 
#include <libusb.h>

// same as in main.c
#define USB_LED_OFF 0
#define USB_LED_ON  1
#define USB_DATA_OUT 2
#define USB_DATA_WRITE 3
#define USB_DATA_IN 4

// used to get descriptor strings for device identification 
static int usbGetDescriptorString(libusb_device_handle *dev, int index, int langid, 
                                  char *buf, int buflen) {
    char buffer[256];
    int rval, i;

    // make standard request GET_DESCRIPTOR, type string and given index 
    // (e.g. dev->iProduct)
    rval = libusb_control_transfer(
        dev, 
        LIBUSB_REQUEST_TYPE_STANDARD | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN, 
        LIBUSB_REQUEST_GET_DESCRIPTOR, (LIBUSB_DT_STRING << 8) + index, langid, 
        buffer, sizeof(buffer), 1000
    );
        
    if(rval < 0) // error
        return rval;
	
    // rval should be bytes read, but buffer[0] contains the actual response size
    if((unsigned char)buffer[0] < rval)
        rval = (unsigned char)buffer[0]; // string is shorter than bytes read
	
    if(buffer[1] != LIBUSB_DT_STRING) // second byte is the data type
        return 0; // invalid return type
		
    // we're dealing with UTF-16LE here so actual chars is half of rval,
    // and index 0 doesn't count
    rval /= 2;
	
    // lossy conversion to ISO Latin1 
    for(i = 1; i < rval && i < buflen; i++) {
        if(buffer[2 * i + 1] == 0)
            buf[i-1] = buffer[2 * i];
        else
            buf[i-1] = '?'; // outside of ISO Latin1 range
    }
    buf[i-1] = 0;

    return i-1;
}

static libusb_device_handle * usbOpenDevice(int vendor, char *vendorName, 
                                            int product, char *productName) {
    int e;
    libusb_device *dev;
    char devVendor[256], devProduct[256];
    
    libusb_device_handle * handle = NULL;
    
    libusb_init(NULL);
    
    libusb_device **devlist;
    ssize_t ndevices = libusb_get_device_list(NULL, &devlist);
    for (int i = 0; i < ndevices; ++i) {
        libusb_device *dev = devlist[i];
        struct libusb_device_descriptor desc;

        if ((e = libusb_get_device_descriptor(dev, &desc)) < 0) {
            fprintf(stderr, "Could not get descriptor: %s", libusb_strerror(e));
            continue;
        }
    
        if(desc.idVendor != vendor ||
           desc.idProduct != product)
            continue;
        
        // we need to open the device in order to query strings 
        if((e = libusb_open(dev, &handle)) < 0) {
            fprintf(stderr, "Warning: cannot open USB device: %s\n",
                    libusb_strerror(e));
            continue;
        }
        
        // get vendor name 
        if((e = usbGetDescriptorString(handle, desc.iManufacturer, 0x0409, devVendor, sizeof(devVendor))) < 0) {
            fprintf(stderr, 
                    "Warning: cannot query manufacturer for device: %s\n", 
                    libusb_strerror(e));
            libusb_close(handle);
            continue;
        }
        
        // get product name 
        if((e = usbGetDescriptorString(handle, desc.iProduct, 
                                       0x0409, devProduct, sizeof(devVendor))) < 0) {
            fprintf(stderr, 
                    "Warning: cannot query product for device: %s\n", 
                    libusb_strerror(e));
            libusb_close(handle);
            continue;
        }
        
        if(strcmp(devVendor, vendorName) == 0 && 
           strcmp(devProduct, productName) == 0)
            return handle;
        else
            libusb_close(handle);
    }
    
    return NULL;
}

int main(int argc, char **argv) {
	 libusb_device_handle *handle = NULL;
    int nBytes = 0;
    char buffer[256];

    if(argc < 2) {
        printf("Usage:\n");
        printf("usbtext.exe on\n");
        printf("usbtext.exe off\n");
        printf("usbtext.exe out\n");
        printf("usbtext.exe write\n");
        printf("usbtext.exe in <string>\n");
        exit(1);
    }
	
    handle = usbOpenDevice(0x16C0, "codeandlife.com", 0x05DC, "USBexample");
	
    if(handle == NULL) {
        fprintf(stderr, "Could not find USB device!\n");
        exit(1);
    }
    
    if(strcmp(argv[1], "on") == 0) {
        nBytes = libusb_control_transfer(
            handle, 
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN, 
            USB_LED_ON, 0, 0, buffer, sizeof(buffer), 5000
        );
    } else if(strcmp(argv[1], "off") == 0) {
        nBytes = libusb_control_transfer(
            handle, 
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN, 
            USB_LED_OFF, 0, 0, (char *)buffer, sizeof(buffer), 5000
        );
    } else if(strcmp(argv[1], "out") == 0) {
        nBytes = libusb_control_transfer(
            handle, 
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN, 
            USB_DATA_OUT, 0, 0, (char *)buffer, sizeof(buffer), 5000
        );
        printf("Got %d bytes: %s\n", nBytes, buffer);
    } else if(strcmp(argv[1], "write") == 0) {
        nBytes = libusb_control_transfer(
            handle, 
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN, 
            USB_DATA_WRITE, 'T' + ('E' << 8), 'S' + ('T' << 8), 
            (char *)buffer, sizeof(buffer), 5000
        );
    } else if(strcmp(argv[1], "in") == 0 && argc > 2) {
        nBytes = libusb_control_transfer(
            handle, 
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT, 
            USB_DATA_IN, 0, 0, argv[2], strlen(argv[2])+1, 5000
        );
    }
	
    if(nBytes < 0)
        fprintf(stderr, "USB error: %s\n", libusb_strerror(nBytes));
		
    libusb_close(handle);
	
    return 0;
}
