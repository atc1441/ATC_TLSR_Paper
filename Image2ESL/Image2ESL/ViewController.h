//
//  ViewController.h
//  Image2ESL
//
//  Created by Larry Bank
//  Copyright (c) 2021 BitBank Software Inc. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import "DragDropView.h"
#import "MyBLE.h"

@interface ViewController : NSViewController

@property (nonatomic, retain) DragDropView *myview;
@property (nonatomic) NSString *filename;
@property (weak) IBOutlet NSImageView *myImage;
@property (weak) IBOutlet NSTextField *StatusLabel;
@property (weak) IBOutlet NSButtonCell *DitherCheck;

// Process a new file
- (void)processFile:(NSString *)path;
- (void)ditherFile:(NSNotification *) notification;
- (uint8_t *)DitherImage:(uint8_t*)pPixels width:(int)iWidth height:(int)iHeight pitch:(int)iSrcPitch dither:(bool)bDither;
- (void) sendImage;
@end

