#include "parser.hpp"

#include <sstream>
#include <cstring>

#include <utki/debug.hpp>

#include "Exc.hpp"



using namespace stob;



void Parser::appendCharToString(std::uint8_t c){
	*this->p = c;
	++this->p;
	if(this->p == &*this->buf.end()){
		std::vector<std::uint8_t> a(this->buf.size() * 2);
		memcpy(&*a.begin(), this->buf.begin(), this->buf.sizeInBytes());

		this->p = &*a.begin() + this->buf.size();

		this->arrayBuf = std::move(a);
		this->buf = utki::wrapBuf(this->arrayBuf); //set reference
	}
}



void Parser::handleLeftCurlyBracket(ParseListener& listener){
	if(this->nestingLevel == unsigned(-1)){
		throw stob::Exc("Malformed STOB document. Nesting level is too high.");
	}
	if(!this->stringParsed){
		throw stob::Exc("Malformed STOB document. Curly braces without preceding string declaration encountered.");
	}
	++this->nestingLevel;
	this->stringParsed = false;
	listener.onChildrenParseStarted();
}



void Parser::handleRightCurlyBracket(ParseListener& listener){
	if(this->nestingLevel == 0){
		std::stringstream ss;
		ss << "Malformed STOB document. Unexpected '}' at line: ";
		ss << this->curLine;
		throw stob::Exc(ss.str());
	}
	--this->nestingLevel;
	this->stringParsed = false;
	listener.onChildrenParseFinished();
}



void Parser::handleStringEnd(ParseListener& listener){
	size_t size = this->p - this->buf.begin();
	listener.onStringParsed(utki::Buf<char>(size == 0 ? 0 : reinterpret_cast<char*>(&*this->buf.begin()), size));
	this->arrayBuf.clear();
	this->buf = utki::wrapBuf(this->staticBuf);
	this->p = this->buf.begin();
	this->stringParsed = true;
	this->state = E_State::IDLE;
}



void Parser::parseChar(std::uint8_t c, ParseListener& listener){
	switch(this->state){
		case E_State::IDLE:
			switch(c){
				case '"':
					this->state = E_State::QUOTED_STRING;
					break;
				case '{':
					if(!this->stringParsed){
						//empty string with children
						ASSERT(this->p == this->buf.begin())
						this->handleStringEnd(listener);
					}
					this->handleLeftCurlyBracket(listener);
					break;
				case '}':
					this->handleRightCurlyBracket(listener);
					break;
				case '\n':
					++this->curLine;
					break;
				case '\r':
				case ' ':
				case '\t':
					//ignore
					break;
				default:
					this->state = E_State::UNQUOTED_STRING;
					this->appendCharToString(c);
					break;
			}
			break;
		case E_State::UNQUOTED_STRING:
			switch(c){
				case '\n':
					++this->curLine;
				case ' ':
				case '\r':
				case '\t':
				case '{':
				case '}':
				case '"':
					this->handleStringEnd(listener);
					
					switch(c){
						case '{':
							this->handleLeftCurlyBracket(listener);
							break;
						case '}':
							this->handleRightCurlyBracket(listener);
							break;
						case '"':
							//start parsing quoted string right a way
							this->state = E_State::QUOTED_STRING;
							this->stringParsed = false;
							break;
						default:
							break;
					}
					return;
					
				default:
					this->appendCharToString(c);
					break;
			}
			break;
		case E_State::QUOTED_STRING:
			this->appendCharToString(c);
			break;
		default:
			ASSERT(false)
			break;
	}
}



void Parser::preParseChar(std::uint8_t c, ParseListener& listener){
	if(this->prevChar != 0){
		switch(this->state){
			case E_State::IDLE:
			case E_State::UNQUOTED_STRING:
				ASSERT(this->prevChar == '/')//possible comment sequence
				switch(c){
					case '/':
						this->commentState = E_CommentState::LINE_COMMENT;
						break;
					case '*':
						this->commentState = E_CommentState::MULTILINE_COMMENT;
						break;
					default:
						this->parseChar('/', listener);
						this->parseChar(c, listener);
						break;
				}
				break;
			case E_State::QUOTED_STRING:
				ASSERT(this->prevChar == '\\')//escape sequence
				switch(c){
					case '\\'://backslash
						this->parseChar('\\', listener);
						break;
					case '/'://slash
						this->parseChar('/', listener);
						break;
					case '"':
						this->parseChar('"', listener);
						break;
					case 'n':
						this->parseChar('\n', listener);
						break;
					case 'r':
						this->parseChar('\r', listener);
						break;
					case 't':
						this->parseChar('\t', listener);
						break;
					default:
						{
							std::stringstream ss;
							ss << "Malformed document. Unknown escape sequence (\\" << c << ") on line: ";
							ss << this->curLine;
							throw stob::Exc(ss.str());
						}
						break;
				}
				break;
			default:
				ASSERT(false)
				break;
		}
		this->prevChar = 0;
	}else{//~if(this->prevChar != 0)
		switch(this->state){
			case E_State::QUOTED_STRING:
				switch(c){
					case '\\': //escape sequence
						this->prevChar = '\\';
						break;
					case '"':
//							TRACE(<< "qsp = " << std::string(reinterpret_cast<char*>(this->buf->Begin()), 11) << std::endl)
						this->handleStringEnd(listener);
						break;
					case '\n':
						++this->curLine;
					case '\r':
					case '\t':
						//ignore
						break;
					default:
						this->parseChar(c, listener);
						break;
				}
				break;
			case E_State::UNQUOTED_STRING:
			case E_State::IDLE:
				if(c == '/'){//possible comment sequence
					this->prevChar = '/';
				}else{
					this->parseChar(c, listener);
				}
				break;
			default:
				ASSERT(false)
				break;
		}//~switch
	}
}



void Parser::parseDataChunk(const utki::Buf<std::uint8_t> chunk, ParseListener& listener){
	for(const std::uint8_t* s = chunk.begin(); s != chunk.end(); ++s){
//		TRACE(<< "Parser::ParseDataChunk(): *s = " << (*s) << std::endl)
		
		//skip comments if needed
		if(this->commentState != E_CommentState::NO_COMMENT){
			switch(this->commentState){
				case E_CommentState::LINE_COMMENT:
					for(;; ++s){
						if(s == chunk.end()){
							return;
						}
						if(*s == '\n'){
							++this->curLine;
							this->commentState = E_CommentState::NO_COMMENT;
							break;//~for
						}
					}
					break;//~switch
				case E_CommentState::MULTILINE_COMMENT:
					if(this->prevChar == '*'){
						this->prevChar = 0;
						if(*s == '/'){
							this->commentState = E_CommentState::NO_COMMENT;
							break;//~switch()
						}
					}
					for(;; ++s){
						if(s == chunk.end()){
							return;
						}
						if(*s == '\n'){
							++this->curLine;
						}else if(*s == '*'){
							this->prevChar = '*';
							break;//~for()
						}
					}
					break;//~switch
				default:
					ASSERT(false)
					break;
			}
			continue;
		}
		
		this->preParseChar(*s, listener);
		
	}//~for(s)
}



void Parser::endOfData(ParseListener& listener){
	if(this->state != E_State::IDLE){
		//add new line at the end of data
		this->preParseChar('\n', listener);
	}
	
	if(this->nestingLevel != 0 || this->state != E_State::IDLE){
		throw stob::Exc("Malformed stob document fed. After parsing all the data, the parser remained in the middle of some parsing task.");
	}
	
	this->reset();
}



void stob::parse(const papki::File& fi, ParseListener& listener){
	papki::File::Guard fileGuard(fi);
	
	stob::Parser parser;
	
	std::array<std::uint8_t, 2048> buf; //2kb read buffer.
	
	size_t bytesRead;
	
	do{
		bytesRead = fi.read(utki::wrapBuf(buf));
		
		utki::Buf<std::uint8_t> b(&*buf.begin(), bytesRead);
		parser.parseDataChunk(b, listener);
	}while(bytesRead == buf.size());

	parser.endOfData(listener);
}
