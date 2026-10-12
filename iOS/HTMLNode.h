#import <Foundation/Foundation.h>

@class HTMLDocument;

@interface HTMLNode : NSObject {
    void* node;
    HTMLDocument* document;
}
- (NSString*)tagName;
- (NSString*)text;
- (NSString*)attr:(NSString*)name;
- (BOOL)hasClass:(NSString*)className;
- (HTMLNode*)parent;
- (HTMLNode*)nextElementSibling;
- (HTMLNode*)selectFirst:(NSString*)tagName;
- (NSArray*)select:(NSString*)tagName;
@end

@interface HTMLDocument : NSObject {
    void* doc;
}
+ (HTMLDocument*)parse:(NSString*)html baseURI:(NSString*)baseURI;
+ (HTMLDocument*)parseData:(NSData*)data baseURI:(NSString*)baseURI;
- (HTMLNode*)root;
- (HTMLNode*)selectFirst:(NSString*)tagName;
- (NSArray*)select:(NSString*)tagName;
@end
