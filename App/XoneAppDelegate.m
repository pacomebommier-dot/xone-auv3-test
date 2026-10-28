#import "XoneAppDelegate.h"

@implementation XoneAppDelegate

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    (void)application; (void)launchOptions;
    self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    UIViewController *screen = [UIViewController new];
    screen.view.backgroundColor = [UIColor colorWithRed:0.055 green:0.075 blue:0.105 alpha:1];

    UILabel *label = [UILabel new];
    label.translatesAutoresizingMaskIntoConstraints = NO;
    label.numberOfLines = 0;
    label.textAlignment = NSTextAlignmentCenter;
    label.textColor = UIColor.whiteColor;
    label.font = [UIFont systemFontOfSize:22 weight:UIFontWeightSemibold];
    label.text = @"XONE ENGINE · V0.2\n\nApplication installée.\n\nOuvrez AUM, créez une tranche MIDI et recherchez\nXONE ENGINE V0.2 parmi les AUv3 MIDI Processors.\n\nK2 ou simulateur CH15 → XONE → moniteur MIDI.\nRecette simulée disponible dans l’interface du plugin.\nLes instruments et profils restent à configurer.";
    [screen.view addSubview:label];
    [NSLayoutConstraint activateConstraints:@[
        [label.leadingAnchor constraintEqualToAnchor:screen.view.safeAreaLayoutGuide.leadingAnchor constant:24],
        [label.trailingAnchor constraintEqualToAnchor:screen.view.safeAreaLayoutGuide.trailingAnchor constant:-24],
        [label.centerYAnchor constraintEqualToAnchor:screen.view.centerYAnchor]
    ]];
    self.window.rootViewController = screen;
    [self.window makeKeyAndVisible];
    return YES;
}
@end
