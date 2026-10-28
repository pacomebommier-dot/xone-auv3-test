#import <AudioToolbox/AudioToolbox.h>

@interface XoneMidiAudioUnit : AUAudioUnit
- (BOOL)loadAumValidationFixture; // explicit simulation only, serialized off render
- (NSString *)drainDiagnosticText; // sole consumer, call on main thread
@end
