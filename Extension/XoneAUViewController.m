#import <AudioToolbox/AudioToolbox.h>
#import <UIKit/UIKit.h>
#import <CoreAudioKit/CoreAudioKit.h>
#import "XoneMidiAudioUnit.h"

@interface XoneAUViewController : AUViewController <AUAudioUnitFactory>
@property (atomic, strong) XoneMidiAudioUnit *xoneUnit;
@property (nonatomic, strong) UISwitch *presetSwitch;
@property (nonatomic, strong) UIButton *refreshButton;
@property (nonatomic, strong) UIButton *fixtureButton;
@property (nonatomic, strong) UIStackView *diagnostics;
@property (nonatomic, strong) UIStackView *contentStack;
@property (nonatomic, strong) UILabel *traceLabel;
@property (nonatomic, strong) NSTimer *traceTimer;
@property (nonatomic, strong) NSMutableString *traceHistory;
@end

@implementation XoneAUViewController
- (AUAudioUnit *)createAudioUnitWithComponentDescription:(AudioComponentDescription)desc
                                                   error:(NSError * _Nullable __autoreleasing *)error {
    XoneMidiAudioUnit *unit=[[XoneMidiAudioUnit alloc] initWithComponentDescription:desc options:0 error:error];
    self.xoneUnit=unit;
    return unit;
}
- (UILabel *)label:(NSString *)text {
    UILabel *label=[UILabel new];label.text=text;label.numberOfLines=0;label.textColor=UIColor.whiteColor;
    label.font=[UIFont preferredFontForTextStyle:UIFontTextStyleBody];label.adjustsFontForContentSizeCategory=YES;
    return label;
}
- (UIStackView *)row:(NSString *)title control:(UIView *)control {
    UIStackView *row=[[UIStackView alloc] initWithArrangedSubviews:@[[self label:title],control]];
    row.axis=UILayoutConstraintAxisHorizontal;row.spacing=12;row.alignment=UIStackViewAlignmentCenter;
    return row;
}
- (void)loadView {
    UIView *root=[[UIView alloc] initWithFrame:CGRectMake(0,0,360,360)];
    root.backgroundColor=[UIColor colorWithRed:0.055 green:0.075 blue:0.105 alpha:1];
    UIScrollView *scroll=[UIScrollView new];scroll.translatesAutoresizingMaskIntoConstraints=NO;[root addSubview:scroll];
    UIStackView *stack=[UIStackView new];stack.axis=UILayoutConstraintAxisVertical;stack.spacing=14;stack.translatesAutoresizingMaskIntoConstraints=NO;
    [scroll addSubview:stack];self.contentStack=stack;
    UILabel *title=[self label:@"XONE · MIDI universel"];title.font=[UIFont preferredFontForTextStyle:UIFontTextStyleTitle2];[stack addArrangedSubview:title];
    [stack addArrangedSubview:[self label:@"Une entrée K2 ou K2_SIM. Le routage et les instruments se règlent dans l’hôte."]];
    self.presetSwitch=[UISwitch new];self.presetSwitch.accessibilityLabel=@"Éditeur PRESET";
    [self.presetSwitch addTarget:self action:@selector(changePresetPage:) forControlEvents:UIControlEventValueChanged];
    [stack addArrangedSubview:[self row:@"Éditeur PRESET" control:self.presetSwitch]];
    self.refreshButton=[UIButton buttonWithType:UIButtonTypeSystem];
    [self.refreshButton setTitle:@"Rafraîchir les LED" forState:UIControlStateNormal];
    [self.refreshButton addTarget:self action:@selector(refreshControllerLEDs) forControlEvents:UIControlEventTouchUpInside];
    [stack addArrangedSubview:self.refreshButton];
    UISwitch *diagnostic=[UISwitch new];diagnostic.accessibilityLabel=@"Afficher le diagnostic";
    [diagnostic addTarget:self action:@selector(showDiagnostics:) forControlEvents:UIControlEventValueChanged];
    [stack addArrangedSubview:[self row:@"Diagnostic" control:diagnostic]];
    self.diagnostics=[UIStackView new];self.diagnostics.axis=UILayoutConstraintAxisVertical;self.diagnostics.spacing=10;
    [self.diagnostics addArrangedSubview:[self label:@"Simulation : session isolée, moniteur seul. Le bouton remplace le LIVE par l’état de test ; aucun instrument n’est configuré."]];
    self.fixtureButton=[UIButton buttonWithType:UIButtonTypeSystem];
    [self.fixtureButton setTitle:@"Charger la simulation de test" forState:UIControlStateNormal];
    [self.fixtureButton addTarget:self action:@selector(loadValidationFixture) forControlEvents:UIControlEventTouchUpInside];
    [self.diagnostics addArrangedSubview:self.fixtureButton];
    self.traceHistory=[NSMutableString stringWithString:@"Trace logicielle. SENT confirme le callback MIDI, pas la réception instrumentale. STOP et vrais UP avant retrait.\n\n"];
    self.traceLabel=[self label:self.traceHistory];self.traceLabel.font=[UIFont monospacedSystemFontOfSize:12 weight:UIFontWeightRegular];
    [self.diagnostics addArrangedSubview:self.traceLabel];
    [NSLayoutConstraint activateConstraints:@[
        [scroll.topAnchor constraintEqualToAnchor:root.safeAreaLayoutGuide.topAnchor],
        [scroll.bottomAnchor constraintEqualToAnchor:root.safeAreaLayoutGuide.bottomAnchor],
        [scroll.leadingAnchor constraintEqualToAnchor:root.leadingAnchor],[scroll.trailingAnchor constraintEqualToAnchor:root.trailingAnchor],
        [stack.topAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.topAnchor constant:16],
        [stack.bottomAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.bottomAnchor constant:-16],
        [stack.leadingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.leadingAnchor constant:16],
        [stack.trailingAnchor constraintEqualToAnchor:scroll.contentLayoutGuide.trailingAnchor constant:-16],
        [stack.widthAnchor constraintEqualToAnchor:scroll.frameLayoutGuide.widthAnchor constant:-32],
        [self.refreshButton.heightAnchor constraintGreaterThanOrEqualToConstant:44],
        [self.fixtureButton.heightAnchor constraintGreaterThanOrEqualToConstant:44]
    ]];
    self.view=root;[self refreshControls];
}
- (void)refreshControls {
    XoneMidiAudioUnit *unit=self.xoneUnit;AUParameter *preset=[unit.parameterTree parameterWithAddress:0];
    self.presetSwitch.enabled=preset!=nil;self.refreshButton.enabled=[unit.parameterTree parameterWithAddress:1]!=nil;self.fixtureButton.enabled=unit!=nil;
    [self.presetSwitch setOn:preset.value>=0.5f animated:NO];
}
- (void)changePresetPage:(UISwitch *)sender {
    AUParameter *preset=[self.xoneUnit.parameterTree parameterWithAddress:0];
    if(preset)preset.value=sender.isOn?1:0;else [sender setOn:NO animated:NO];
}
- (void)refreshControllerLEDs {
    AUParameter *refresh=[self.xoneUnit.parameterTree parameterWithAddress:1];if(refresh)refresh.value=1;
}
- (void)showDiagnostics:(UISwitch *)sender {
    if(sender.isOn){if(!self.diagnostics.superview)[self.contentStack addArrangedSubview:self.diagnostics];}
    else {[self.contentStack removeArrangedSubview:self.diagnostics];[self.diagnostics removeFromSuperview];}
}
- (void)loadValidationFixture {
    const BOOL accepted=[self.xoneUnit loadAumValidationFixture];
    [self.traceHistory appendString:accepted?@"Simulation demandée ; application au rendu après vrais UP et OFF dus.\n":@"Simulation refusée ; aucun état remplacé.\n"];
    self.traceLabel.text=self.traceHistory;
}
- (void)viewDidAppear:(BOOL)animated {
    [super viewDidAppear:animated];[self.traceTimer invalidate];[self refreshControls];
    __weak XoneAUViewController *weakSelf=self;
    self.traceTimer=[NSTimer scheduledTimerWithTimeInterval:0.25 repeats:YES block:^(NSTimer *timer){
        (void)timer;XoneAUViewController *owner=weakSelf;if(!owner)return;
        XoneMidiAudioUnit *unit=owner.xoneUnit;if(!unit)return;
        [owner refreshControls];[owner.traceHistory appendString:[unit drainDiagnosticText]];
        if(owner.traceHistory.length>24000)[owner.traceHistory deleteCharactersInRange:NSMakeRange(0,owner.traceHistory.length-16000)];
        owner.traceLabel.text=owner.traceHistory;
    }];
}
- (void)viewDidDisappear:(BOOL)animated {
    [self.traceTimer invalidate];self.traceTimer=nil;[super viewDidDisappear:animated];
}
- (void)dealloc {[_traceTimer invalidate];}
@end
