//
//  MyBLE.m
//  Image2ESL
//
//  Created by Larry Bank
//  Copyright (c) 2021 BitBank Software Inc. All rights reserved.
//

#import "MyBLE.h"

static NSString *ESLService = @"13187b10-eba9-a3ba-044e-83d3217d9a38";
static NSString *ESLChar = @"4b646063-6264-f3a7-8941-e65356ea82fe";

@implementation MyBLE

- (instancetype)init
{
    _bConnected = false;
    self.centralManager = [[CBCentralManager alloc] initWithDelegate:self queue:nil];
//    return [self initWithQueue:nil];
    self.discoveredPeripherals = [NSMutableArray array];
    self = [super init];
    return self;
}

- (void)dealloc
{
    [self.centralManager stopScan];
    //    [_manager dealloc]
}

- (bool)isConnected
{
    return _bConnected;
} /* isConnected */

- (NSString *)getName
{
    return [_myPeripheral name];
} /* getName */

- (void)centralManager:(CBCentralManager *)central
 didDiscoverPeripheral:(CBPeripheral *)aPeripheral
     advertisementData:(NSDictionary *)advertisementData
                  RSSI:(NSNumber *)RSSI
{
    NSMutableArray *peripherals =  [self mutableArrayValueForKey:@"discoveredPeripherals"];
    const char* deviceName = [[aPeripheral name] cStringUsingEncoding:NSASCIIStringEncoding];
    
    if (deviceName) printf("Found device: %s\n", deviceName);
    if( deviceName && ![self.discoveredPeripherals containsObject:aPeripheral])
    {
        // check if it's one of the supported name
        if (memcmp(deviceName, "ESL", 3) == 0) {
            printf("Found a supported e-ink label: %s, connecting...\n", deviceName);
            [peripherals addObject:aPeripheral];
            [self.discoveredPeripherals addObject:aPeripheral];
            [self connectToPeripheral: aPeripheral];
        }
    }
}

//------------------------------------------------------------------------------
- (void) centralManager: (CBCentralManager *)central
   didConnectPeripheral: (CBPeripheral *)aPeripheral
{
    [aPeripheral setDelegate:self];
    [aPeripheral discoverServices:nil];
}

- (void) centralManager:(CBCentralManager *)central
 didRetrievePeripherals:(NSArray *)peripherals
{
    NSLog(@"Retrieved peripheral: %lu - %@", [peripherals count], peripherals);
    
    [self.centralManager stopScan];
    
    /* If there are any known devices, automatically connect to it.*/
    if([peripherals count] >=1)
    {
        _myPeripheral = [peripherals objectAtIndex:0];
        [self.centralManager connectPeripheral:_myPeripheral
                            options:nil];
    }
}

//------------------------------------------------------------------------------
- (void)centralManagerDidUpdateState:(CBCentralManager *)manager
{
}
//------------------------------------------------------------------------------
// Invoked whenever an existing connection with the peripheral is torn down.
- (void) centralManager: (CBCentralManager *)central
didDisconnectPeripheral: (CBPeripheral *)aPeripheral
                  error: (NSError *)error
{
    printf("didDisconnectPeripheral\n");
    _bConnected = NO;
    [[NSNotificationCenter defaultCenter] postNotificationName:@"StatusChangedNotification"
                                                        object:self userInfo:nil];

}
//------------------------------------------------------------------------------
/// Invoked whenever the central manager fails to create a connection with the peripheral.
- (void) centralManager: (CBCentralManager *)central didFailToConnectPeripheral:(CBPeripheral *)aPeripheral
                  error:(NSError *)error
{
    NSLog(@"Fail to connect to peripheral: %@ with error = %@", aPeripheral, [error localizedDescription]);
}
//------------------------------------------------------------------------------
- (void) startScan
{
    NSLog(@"Start scanning");
    
    [self.centralManager scanForPeripheralsWithServices: nil options: nil];
} /* startScan */

- (void) connectToPeripheral: (CBPeripheral *)aPeripheral
{
    [self.centralManager stopScan];
    _myPeripheral = aPeripheral;
    NSDictionary *connectOptions = @{
        CBConnectPeripheralOptionNotifyOnConnectionKey: @YES,
        CBConnectPeripheralOptionNotifyOnDisconnectionKey: @YES,
        CBConnectPeripheralOptionNotifyOnNotificationKey: @YES,
        //        CBConnectPeripheralOptionEnableTransportBridgingKey:,
        //        CBConnectPeripheralOptionRequiresANCS:,
        CBConnectPeripheralOptionStartDelayKey: @0
    };
    [self.centralManager connectPeripheral:_myPeripheral options:connectOptions];
}

// Invoked upon completion of a -[discoverServices:] request.
// Discover available characteristics on interested services
- (void) peripheral: (CBPeripheral *)aPeripheral
didDiscoverServices:(NSError *)error
{
    for (CBService *aService in aPeripheral.services)
    {
        NSLog(@"Service found with UUID: %@", aService.UUID);
        [aPeripheral discoverCharacteristics:nil forService:aService];
    }
}
//------------------------------------------------------------------------------

// Invoked upon completion of a -[discoverCharacteristics:forService:] request.
// Perform appropriate operations on interested characteristics
- (void) peripheral: (CBPeripheral *)aPeripheral didDiscoverCharacteristicsForService:(CBService *)service error:(NSError *)error
{
    for (CBCharacteristic *aChar in service.characteristics)
    {
        NSLog(@"Service: %@ with Char: %@", [aChar service].UUID, aChar.UUID);
        CBUUID *theSvc = [CBUUID UUIDWithString:ESLService];
        CBUUID *theChr = [CBUUID UUIDWithString:ESLChar];
        if ([[aChar service].UUID isEqual:theSvc] && [aChar.UUID isEqual:theChr]) {
            printf("Found the service+char we're looking for!\n");
        _myChar = aChar; // keep these since we will need them for communicating
        _bConnected = YES; // indicates that we're ready to send data
            [[NSNotificationCenter defaultCenter] postNotificationName:@"StatusChangedNotification"
                                                                object:self userInfo:nil];
        }
    }
}

// Invoked upon completion of a -[readValueForCharacteristic:] request or on the reception of a notification/indication.
- (void) peripheral: (CBPeripheral *)aPeripheral didUpdateValueForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
    [self printCharacteristicData:characteristic];
}

- (void) printCharacteristicData: (CBCharacteristic *)characteristic
{
#if DEBUG_MODE
    NSLog(@"Read Characteristics: %@", characteristic.UUID);
    NSLog(@"%@", [characteristic description]);
#endif
    NSData * updatedValue = characteristic.value;
    printf("%s\n",(char*)updatedValue.bytes);
}

- (void) peripheral: (CBPeripheral *)peripheral didDiscoverDescriptorsForCharacteristic:(CBDescriptor *)descriptor error:(NSError *)error
{
    
}

- (void)peripheral: (CBPeripheral *)peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic *)characteristic error:(NSError *)error
{
    
}
- (void)peripheral: (CBPeripheral *)peripheral didModifyServices:(NSArray<CBService *> *)invalidatedServices
{
    exit(0);
}

- (void)writeData: (uint8_t *)pData withLength:(int)len withResponse:(bool)response
{
    NSData *myData = [NSData dataWithBytes:pData length:len];
    
    if (response)
        [_myPeripheral writeValue:myData forCharacteristic:_myChar type:CBCharacteristicWriteWithResponse];
    else
        [_myPeripheral writeValue:myData forCharacteristic:_myChar type:CBCharacteristicWriteWithoutResponse];

} /* writeData */

@end
