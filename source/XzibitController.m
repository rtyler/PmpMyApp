//
//  XzibitController.m
//  PmpMyApp
//
//  Created by R. Tyler Ballance on 2/9/07.
//  Copyright 2007 bleep. LLC. All rights reserved.
//

#import "XzibitController.h"
#include "pmpmapper.h"

@implementation XzibitController

- (IBAction)createMapping:(id)sender
{
	int privateport = [localCreatePort intValue];
	int publicport = [natCreatePort intValue];
	int duration = [mappingDuration intValue];
	
	if (pmp_create_map(PMP_MAP_TCP,privateport,publicport,(duration * 60)) != NULL)
	{
		[statusField setStringValue:[NSString stringWithFormat:@"Mapping for port %d created with external port %d", privateport, publicport]];
	}
	else
	{
		[statusField setStringValue:@"Failed to properly create the mapping"];
	}
}

- (IBAction)destroyMapping:(id)sender
{
	int privateport = [localDestroyPort intValue];
	
	if (pmp_destroy_map(PMP_MAP_TCP,privateport) != NULL)
	{
		[statusField setStringValue:[NSString stringWithFormat:@"Mapping for port %d destroyed", privateport]];
	}
	else
	{
		[statusField setStringValue:@"Failed to properly destroy the mapping"];
	}
}

@end
