#import <AudioToolbox/AudioToolbox.h>

@interface XoneMidiAudioUnit : AUAudioUnit
- (NSString *)drainDiagnosticText; // sole consumer, call on main thread
@end
