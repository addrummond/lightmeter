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

#include <usbconstants.h>
#include <exposure.h>

#include <assert.h>

// used to get descriptor strings for device identification 
static int usbGetDescriptorString(libusb_device_handle *dev, int index, int langid, 
                                  char *buf, int buflen) {
    unsigned char buffer[256];
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
    unsigned char buffer[256];

    if(argc < 2) {
        fprintf(stderr, "Requires argument\n");
        return 1;
    }

    handle = usbOpenDevice(0x16c0, "ibexsoft", 0x05DC, "luxatron");
	
    if(handle == NULL) {
        fprintf(stderr, "Could not find USB device!\n");
        exit(1);
    }
    
    printf("Device found...\n");

    if (!strcmp(argv[1], "test")) {
        nBytes = libusb_control_transfer(
            handle,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN/*device to host */,
            USB_BREQUEST_GET_TEST_MSG,
            0, 0,   // wvalue, windex
            buffer,
            64,     // how much to read
            5000    // 5000ms timeout
        );
        if (nBytes < 0) {
            fprintf(stderr, "%s", libusb_strerror(nBytes));
            return 1;
        }
        printf("Data received %i bytes: [", nBytes);
        for (int i = 0; i < nBytes; ++i) {
            printf("%c", buffer[i]);
        }
        printf("]\n");
    }
    else if (!strcmp(argv[1], "getev")) {
        nBytes = libusb_control_transfer(
            handle,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN/*device to host */,
            USB_BREQUEST_GET_EV,
            0, 0,   // wvalue, windex
            buffer,
            1,      // how much to read
            5000    // 5000ms timeout
        );
        if (nBytes < 0) {
            fprintf(stderr, "%s", libusb_strerror(nBytes));
            return 1;
        }
        assert(nBytes == 1);

        printf("EV: %f\n", ((float)(buffer[0]-(5*8)))/8.0);
    }
    else if (!strcmp(argv[1], "getrawtemp")) {
        nBytes = libusb_control_transfer(
            handle,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN/*device to host */,
            USB_BREQUEST_GET_RAW_TEMPERATURE,
            0, 0,   // wvalue, windex
            buffer,
            2,      // how much to read
            5000    // 5000ms timeout
        );
        if (nBytes < 0) {
            fprintf(stderr, "%s", libusb_strerror(nBytes));
            return 1;
        }
        assert(nBytes == 2);

        printf("Raw temp: %i\n", (int)buffer[0] | (((int)buffer[1]) << 8));
    }
    else if (!strcmp(argv[1], "getrawlight")) {
        nBytes = libusb_control_transfer(
            handle,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN/*device to host */,
            USB_BREQUEST_GET_RAW_LIGHT,
            0, 0,   // wvalue, windex
            buffer,
            2,      // how much to read
            5000    // 5000ms timeout
        );
        if (nBytes < 0) {
            fprintf(stderr, "%s", libusb_strerror(nBytes));
            return 1;
        }
        assert(nBytes == 2);

        printf("Raw light: %i\n", (int)buffer[0] | (((int)buffer[1]) << 8));
    }
    else if (!strcmp(argv[1], "getstopsiso")) {
        nBytes = libusb_control_transfer(
            handle,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN/*device to host */,
            USB_BREQUEST_GET_STOPS_ISO,
            0, 0,   // wvalue, windex
            buffer,
            1,      // how much to read
            5000    // 5000ms timeout
        );
        if (nBytes < 0) {
            fprintf(stderr, "%s", libusb_strerror(nBytes));
            return 1;
        }
        assert(nBytes == 1);

        printf("ISO in stops from ISO 6: %i\n", (int)buffer[0]);
    }
    else if (!strcmp(argv[1], "shutter15") || !strcmp(argv[1], "shutter250")) {
        nBytes = libusb_control_transfer(
            handle,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN/*device to host */,
            USB_BREQUEST_SET_SHUTTER_SPEED,
            argv[1][7] == '1' ? 80 /*1/15*/ : 112 /*1/200*/, 0,
            buffer,
            0,
            5000
        );
        if (nBytes < 0) {
            fprintf(stderr, "%s", libusb_strerror(nBytes));
            return 1;
        }

        printf("Shutter speed set to %s\n", argv[1][7] == '1' ? "1/15" : "1/250");
    }
    else if (!strcmp(argv[1], "ap")) {
        aperture_string_output_t aso;
        nBytes = libusb_control_transfer(
            handle,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_IN/*device to host */,
            USB_BREQUEST_GET_SHUTTER_PRIORITY_EXPOSURE,
            0, 0,
            buffer,
            sizeof(aso.chars),
            5000
        );
        if (nBytes < 0) {
            fprintf(stderr, "%s", libusb_strerror(nBytes));
            return 1;
        }

        assert(nBytes = sizeof(aso.chars));

        printf("Aperture is %s\n", buffer);
    }
    else if (!strcmp(argv[1], "setiso")) {
        uint8_t len = strlen(argv[2]);
        nBytes = libusb_control_transfer(
            handle,
            LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE | LIBUSB_ENDPOINT_OUT/*host to device */,
            USB_BREQUEST_SET_ISO,
            0, 0,
            (uint8_t *)(argv[2]),
            len,
            5000
        );
        if (nBytes < 0) {
            fprintf(stderr, "USB ERROR: %s\n", libusb_strerror(nBytes));
            return 1;
        }

        printf("ISO set\n");
    }
    else {
        fprintf(stderr, "Unrecognized command\n");
    }

    libusb_close(handle);
	
    return 0;
}
