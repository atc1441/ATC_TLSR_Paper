//
//  DragDropView.m
//  Image2ESL
//
//  Created by Larry Bank
//  Copyright (c) 2021 BitBank Software Inc. All rights reserved.
//

#import "DragDropView.h"
#import "ViewController.h"

@implementation DragDropView


- (id)initWithFrame:(NSRect)frame{
    self = [super initWithFrame:frame];
    if (self) {
        [self registerForDraggedTypes:[NSArray arrayWithObject:NSPasteboardTypeFileURL]];
    }
    return self;
}

- (NSDragOperation)draggingEntered:(id <NSDraggingInfo>)sender{
    [self setNeedsDisplay: YES];
    return NSDragOperationGeneric;
}

- (void)draggingExited:(id <NSDraggingInfo>)sender{
    [self setNeedsDisplay: YES];
}

- (BOOL)prepareForDragOperation:(id <NSDraggingInfo>)sender {
    [self setNeedsDisplay: YES];
    return YES;
}

- (BOOL)performDragOperation:(id <NSDraggingInfo>)sender
{
    NSPasteboard *pboard = [sender draggingPasteboard];
    
    if ( [[pboard types] containsObject:NSPasteboardTypeFileURL] ) {
        NSArray<Class> *classes = @[[NSURL class]];
        NSDictionary *options = @{};
        NSArray<NSURL*> *files = [pboard readObjectsForClasses:classes options:options];
        for (NSURL *url in files) {
            NSString *str = [url path];
            if (str != nil)
                [_myVC processFile: str];
        }
    }
    return YES;
}

- (void)concludeDragOperation:(id <NSDraggingInfo>)sender{
    [[NSNotificationCenter defaultCenter] postNotificationName:@"PrintFileNotification"
                                                        object:self userInfo:nil];
}

@end
