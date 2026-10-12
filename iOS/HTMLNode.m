#import "HTMLNode.h"

#include <libxml/HTMLparser.h>
#include <libxml/tree.h>

// AI-generated code

@interface HTMLNode ()
+ (HTMLNode *)nodeWith:(xmlNodePtr)node document:(HTMLDocument *)document;
- (xmlNodePtr)xmlNode;
@end

static NSString *StringFromXML(const xmlChar *value) {
    if (!value) return nil;
    return [NSString stringWithUTF8String:(const char *)value];
}

static void AppendNodeText(xmlNodePtr node, NSMutableString *output) {
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type == XML_TEXT_NODE || child->type == XML_CDATA_SECTION_NODE) {
            NSString *text = StringFromXML(child->content);
            if (text) [output appendString:text];
        } else if (child->type == XML_ELEMENT_NODE) {
            if (xmlStrcasecmp(child->name, (const xmlChar *)"br") == 0) [output appendString:@" "];
            AppendNodeText(child, output);
        }
    }
}

static NSString *NormalizeWhitespace(NSString *text) {
    NSMutableString *output = [NSMutableString stringWithCapacity:[text length]];
    BOOL lastWasSpace = YES;
    for (NSUInteger i = 0; i < [text length]; i++) {
        unichar character = [text characterAtIndex:i];
        BOOL isSpace = (character == ' ' || character == '\t' || character == '\n' ||
                        character == '\r' || character == 0x00A0);
        if (isSpace) {
            if (!lastWasSpace) [output appendString:@" "];
            lastWasSpace = YES;
        } else {
            [output appendFormat:@"%C", character];
            lastWasSpace = NO;
        }
    }
    return [output stringByTrimmingCharactersInSet:[NSCharacterSet whitespaceAndNewlineCharacterSet]];
}

static void CollectByTag(xmlNodePtr node, const char *tagName, HTMLDocument *document, NSMutableArray *output, BOOL firstOnly) {
    for (xmlNodePtr child = node->children; child; child = child->next) {
        if (child->type != XML_ELEMENT_NODE) continue;
        if (xmlStrcasecmp(child->name, (const xmlChar *)tagName) == 0) {
            [output addObject:[HTMLNode nodeWith:child document:document]];
            if (firstOnly) return;
        }
        CollectByTag(child, tagName, document, output, firstOnly);
        if (firstOnly && [output count] > 0) return;
    }
}

@implementation HTMLNode

+ (HTMLNode *)nodeWith:(xmlNodePtr)aNode document:(HTMLDocument *)aDocument {
    if (!aNode) return nil;
    HTMLNode *wrapper = [[HTMLNode alloc] init];
    wrapper->node = aNode;
    wrapper->document = aDocument;
    return wrapper;
}

- (xmlNodePtr)xmlNode {
    return (xmlNodePtr)node;
}

- (NSString *)tagName {
    xmlNodePtr self_ = (xmlNodePtr)node;
    return self_ ? StringFromXML(self_->name) : nil;
}

- (NSString *)text {
    if (!node) return nil;
    NSMutableString *raw = [NSMutableString string];
    AppendNodeText((xmlNodePtr)node, raw);
    return NormalizeWhitespace(raw);
}

- (NSString *)attr:(NSString *)name {
    if (!node) return @"";
    xmlChar *value = xmlGetProp((xmlNodePtr)node, (const xmlChar *)[name UTF8String]);
    if (!value) return @"";
    NSString *result = StringFromXML(value);
    xmlFree(value);
    return result ? result : @"";
}

- (BOOL)hasClass:(NSString *)className {
    NSString *classes = [self attr:@"class"];
    if ([classes length] == 0) return NO;
    for (NSString *component in [classes componentsSeparatedByCharactersInSet:
                                 [NSCharacterSet whitespaceAndNewlineCharacterSet]]) {
        if ([component caseInsensitiveCompare:className] == NSOrderedSame) return YES;
    }
    return NO;
}

- (HTMLNode *)parent {
    xmlNodePtr self_ = (xmlNodePtr)node;
    if (!self_ || !self_->parent || self_->parent->type != XML_ELEMENT_NODE) return nil;
    return [HTMLNode nodeWith:self_->parent document:document];
}

- (HTMLNode *)nextElementSibling {
    xmlNodePtr sibling = ((xmlNodePtr)node)->next;
    while (sibling && sibling->type != XML_ELEMENT_NODE) sibling = sibling->next;
    return sibling ? [HTMLNode nodeWith:sibling document:document] : nil;
}

- (HTMLNode *)selectFirst:(NSString *)tagName {
    if (!node) return nil;
    NSMutableArray *found = [NSMutableArray arrayWithCapacity:1];
    CollectByTag((xmlNodePtr)node, [tagName UTF8String], document, found, YES);
    return [found count] ? [found objectAtIndex:0] : nil;
}

- (NSArray *)select:(NSString *)tagName {
    if (!node) return [NSArray array];
    NSMutableArray *found = [NSMutableArray array];
    CollectByTag((xmlNodePtr)node, [tagName UTF8String], document, found, NO);
    return found;
}

@end

@implementation HTMLDocument

+ (HTMLDocument *)parse:(NSString *)html baseURI:(NSString *)baseURI {
    return [self parseData:[html dataUsingEncoding:NSUTF8StringEncoding] baseURI:baseURI];
}

+ (HTMLDocument *)parseData:(NSData *)data baseURI:(NSString *)baseURI {
    if ([data length] == 0) return nil;
    int options = HTML_PARSE_RECOVER | HTML_PARSE_NOERROR | HTML_PARSE_NOWARNING | HTML_PARSE_NONET;
    xmlDocPtr parsed = htmlReadMemory((const char *)[data bytes], (int)[data length],
                                      baseURI ? [baseURI UTF8String] : "", "UTF-8", options);
    if (!parsed) return nil;
    HTMLDocument *document = [[HTMLDocument alloc] init];
    document->doc = parsed;
    return document;
}

- (HTMLNode *)root {
    xmlNodePtr rootElement = xmlDocGetRootElement((xmlDocPtr)doc);
    return rootElement ? [HTMLNode nodeWith:rootElement document:self] : nil;
}

- (HTMLNode *)selectFirst:(NSString *)tagName {
    return [[self root] selectFirst:tagName];
}

- (NSArray *)select:(NSString *)tagName {
    return [[self root] select:tagName];
}

- (void)dealloc {
    if (doc) xmlFreeDoc((xmlDocPtr)doc);
}

@end
