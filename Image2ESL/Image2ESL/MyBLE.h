//
//  MyBLE.h
//  Image2ESL
//
//  Created by Larry Bank
//  Copyright (c) 2021 BitBank Software Inc. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "CoreBluetooth/CoreBluetooth.h"
#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

@interface MyBLE : NSObject <CBCentralManagerDelegate>

@property (nonatomic, strong) CBCentralManager *centralManager;

- (instancetype)init;
- (void)startScan;
- (void)writeData: (uint8_t *)pData withLength:(int)len withResponse:(bool)response;
- (NSString *)getName;
- (bool)isConnected;

@property (retain) NSMutableArray *discoveredPeripherals;
@property (strong, nonatomic) CBCentralManager * manager;
@property (atomic) int count;
@property (nonatomic) dispatch_queue_t bleQueue;
@property (nonatomic) CBPeripheral *myPeripheral;
@property (nonatomic) CBCharacteristic *myChar;
@property (nonatomic) bool bConnected;
@property (copy) NSString *manufacturer;

@end

NS_ASSUME_NONNULL_END
