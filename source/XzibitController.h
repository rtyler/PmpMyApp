//
//  XzibitController.h
//  PmpMyApp
//
//  Created by R. Tyler Ballance on 2/9/07.
//  Copyright 2007 bleep. LLC. All rights reserved.
//

#import <Cocoa/Cocoa.h>


@interface XzibitController : NSObject {
	IBOutlet NSTextField *localCreatePort;
	IBOutlet NSTextField *natCreatePort;
	IBOutlet NSTextField *mappingDuration;
	IBOutlet NSTextField *localDestroyPort;
	IBOutlet NSTextField *statusField;
}

- (IBAction)createMapping:(id)sender;
- (IBAction)destroyMapping:(id)sender;

@end
