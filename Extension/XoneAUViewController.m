#import <AudioToolbox/AudioToolbox.h>
#import <UIKit/UIKit.h>
#import <CoreAudioKit/CoreAudioKit.h>
#import "XoneMidiAudioUnit.h"

@interface XoneAUViewController : AUViewController <AUAudioUnitFactory>
@property (nonatomic, strong) XoneMidiAudioUnit *xoneUnit;
@end

@implementation XoneAUViewController

- (AUAudioUnit *)createAudioUnitWithComponentDescription:(AudioComponentDescription)desc
                                                   error:(NSError * _Nullable __autoreleasing *)error {
    self.xoneUnit = [[XoneMidiAudioUnit alloc] initWithComponentDescription:desc options:0 error:error];
    return self.xoneUnit;
}

- (void)loadView {
    UIView *root = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 360, 340)];
    root.backgroundColor = [UIColor colorWithRed:0.055 green:0.075 blue:0.105 alpha:1];

    UIScrollView *scroll = [UIScrollView new];
    scroll.translatesAutoresizingMaskIntoConstraints = NO;
    [root addSubview:scroll];

    UILabel *info = [UILabel new];
    info.translatesAutoresizingMaskIntoConstraints = NO;
    info.numberOfLines = 0;
    info.textColor = UIColor.whiteColor;
    info.font = [UIFont monospacedSystemFontOfSize:14 weight:UIFontWeightRegular];
    info.text = @"XONE ENGINE · TEST 0.1\n\nAUv3 MIDI Processor (aumi)\n\nCC 4 → CC 74 · canal 1\nCC 8 → CC 71 · canal 1\n\nRoutez uniquement le XONE:K2 en entrée.\nConnectez XONE TEST OUT à un moniteur MIDI.\nPas de commande LED dans cette version.";
    [scroll addSubview:info];

    [NSLayoutConstraint activateConstraints:@[
        [scroll.topAnchor constraintEqualToAnchor:root.safeAreaLayoutGuide.topAnchor],
        [scroll.bottomAnchor constraintEqualToAnchor:root.safeAreaLayoutGuide.bottomAnchor],
        [scroll.leadingAnchor constraintEqualToAnchor:root.leadingAnchor],
        [scroll.trailingAnchor constraintEqualToAnchor:root.trailingAnchor],
        [info.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor constant:20],
        [info.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor constant:-20],
        [info.leadingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor constant:16],
        [info.trailingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor constant:-16],
        [info.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor constant:-32]
    ]];
    self.view = root;
}
@end
