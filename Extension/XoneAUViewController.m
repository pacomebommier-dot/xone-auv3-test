#import <AudioToolbox/AudioToolbox.h>
#import <UIKit/UIKit.h>
#import <CoreAudioKit/CoreAudioKit.h>
#import "XoneMidiAudioUnit.h"

@interface XoneAUViewController : AUViewController <AUAudioUnitFactory>
@property (nonatomic, strong) XoneMidiAudioUnit *xoneUnit;
@property (nonatomic, strong) UILabel *traceLabel;
@property (nonatomic, strong) NSTimer *traceTimer;
@property (nonatomic, strong) NSMutableString *traceHistory;
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
    self.traceLabel = info;
    self.traceHistory = [NSMutableString stringWithString:@"XONE V0 · candidate AUM\nK2 CH15 → S1/S2/S3 → sorties MIDI API.\nLIVE inconnu : action refusée localement.\nRecette simulée : session isolée, moniteur seul.\nLe bouton remplace le LIVE par la fixture de test ; aucun instrument configuré.\nRX_FEEDBACK=UNKNOWN. STOP avant retrait de l’instance.\n\n"];
    UIButton *fixture = [UIButton buttonWithType:UIButtonTypeSystem];
    fixture.translatesAutoresizingMaskIntoConstraints = NO;
    [fixture setTitle:@"Recette simulée : charger l’état test" forState:UIControlStateNormal];
    [fixture addTarget:self action:@selector(loadValidationFixture) forControlEvents:UIControlEventTouchUpInside];
    [root addSubview:fixture];
    info.text = self.traceHistory;
    [scroll addSubview:info];

    [NSLayoutConstraint activateConstraints:@[
        [fixture.topAnchor constraintEqualToAnchor:root.safeAreaLayoutGuide.topAnchor],
        [fixture.centerXAnchor constraintEqualToAnchor:root.centerXAnchor],
        [fixture.heightAnchor constraintEqualToConstant:44],
        [scroll.topAnchor constraintEqualToAnchor:fixture.bottomAnchor],
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
- (void)loadValidationFixture {
    const BOOL accepted = [self.xoneUnit loadAumValidationFixture];
    [self.traceHistory appendString:accepted
        ? @"RECETTE SIMULEE demandée ; application au rendu après vrais UP et OFF dus.\n"
        : @"RECETTE SIMULEE refusée ; aucun état remplacé.\n"];
    self.traceLabel.text = self.traceHistory;
}
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];
    [self.traceTimer invalidate];
    __weak XoneAUViewController *weakSelf = self;
    self.traceTimer = [NSTimer scheduledTimerWithTimeInterval:0.25 repeats:YES block:^(NSTimer *timer) {
        (void)timer;
        XoneAUViewController *owner = weakSelf;
        if (!owner.xoneUnit) return;
        [owner.traceHistory appendString:[owner.xoneUnit drainDiagnosticText]];
        if (owner.traceHistory.length > 24000) [owner.traceHistory deleteCharactersInRange:NSMakeRange(0, owner.traceHistory.length-16000)];
        owner.traceLabel.text = owner.traceHistory;
    }];
}
- (void)viewDidDisappear:(BOOL)animated {
    [self.traceTimer invalidate]; self.traceTimer = nil;
    [super viewDidDisappear:animated];
}
- (void)dealloc { [_traceTimer invalidate]; }
@end
