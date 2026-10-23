#import <AudioToolbox/AudioToolbox.h>
#import <UIKit/UIKit.h>
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
    UIView *root = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 520, 320)];
    root.backgroundColor = [UIColor colorWithRed:0.055 green:0.075 blue:0.105 alpha:1];

    UILabel *title = [[UILabel alloc] initWithFrame:CGRectMake(24, 20, 472, 47)];
    title.text = @"XONE ENGINE · TEST 0.1";
    title.textColor = [UIColor colorWithRed:0.3 green:0.86 blue:0.98 alpha:1];
    title.font = [UIFont boldSystemFontOfSize:24];
    [root addSubview:title];

    UILabel *instructions = [[UILabel alloc] initWithFrame:CGRectMake(24, 85, 472, 200)];
    instructions.text = @"AUv3 MIDI Processor (aumi)\n\nCC 4  → CC 74 · canal 1\nCC 8  → CC 71 · canal 1\n\nConnectez uniquement XONE:K2 en MIDI IN.\nRoutez XONE TEST OUT vers un moniteur MIDI.\nAucune commande LED dans cette version.";
    instructions.numberOfLines = 0;
    instructions.textColor = [UIColor whiteColor];
    instructions.font = [UIFont monospacedSystemFontOfSize:15 weight:UIFontWeightRegular];
    [root addSubview:instructions];
    self.view = root;
}
@end
