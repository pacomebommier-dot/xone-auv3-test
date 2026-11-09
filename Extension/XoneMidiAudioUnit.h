#import <AudioToolbox/AudioToolbox.h>

@interface XoneMidiAudioUnit : AUAudioUnit
- (BOOL)loadAumValidationFixture; // explicit simulation only, serialized off render
- (BOOL)configureSh01aSource:(NSInteger)source channel:(NSInteger)channel; // explicit provisional receipt, off render
- (NSString *)drainDiagnosticText; // sole consumer, call on main thread
@end
