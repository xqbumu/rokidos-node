#include "src/PlayWrap.h"

using namespace v8;
using namespace std;

void EventHandler(uv_async_t* handle) {
  Nan::HandleScope scope;
  PlayWrap* play = static_cast<PlayWrap*>(handle->data);
  int msg = play->msg;

  Local<Value> argv[] = { Nan::New(msg) };
  play->callback->Call(1, argv);
}

PlayWrap::PlayWrap() {
  uv_async_init(
      uv_default_loop()
    , &async
    , EventHandler
  );
  async.data = (void*)this;
  mPlayer = new MediaPlayer();
}

PlayWrap::PlayWrap(const char* type) {
  uv_async_init(
      uv_default_loop()
    , &async
    , EventHandler
  );
  async.data = (void*)this;
  mPlayer = new MediaPlayer(type);
}

PlayWrap::~PlayWrap() {
  destroy();
}

void PlayWrap::notify(int msg, int ext1, int ext2, int from) {
  fprintf(stdout, "mediaplayer: received event \"%d\"\n", msg);
  PlayWrap* play = static_cast<PlayWrap*>(async.data);
  play->msg = msg;
  uv_async_send(&async);
}

void PlayWrap::destroy() {
  if (destroyed == 1)
    return;
  uv_close(reinterpret_cast<uv_handle_t*>(&async), NULL);
  if (mPlayer) {
    delete mPlayer;
    mPlayer = NULL;
  }
  if (callback) {
    delete callback;
    callback = NULL;
  }
  destroyed = 1;
}

NAN_MODULE_INIT(PlayWrap::Init) {
  Local<FunctionTemplate> tmpl = Nan::New<FunctionTemplate>(New);
  tmpl->SetClassName(Nan::New("PlayWrap").ToLocalChecked());
  tmpl->InstanceTemplate()->SetInternalFieldCount(1);

  Nan::SetPrototypeMethod(tmpl, "setDataSource", SetDataSource);
  Nan::SetPrototypeMethod(tmpl, "start", Start);
  Nan::SetPrototypeMethod(tmpl, "stop", Stop);
  Nan::SetPrototypeMethod(tmpl, "pause", Pause);
  Nan::SetPrototypeMethod(tmpl, "resume", Resume);
  Nan::SetPrototypeMethod(tmpl, "seek", Seek);

  Nan::SetAccessor(tmpl->InstanceTemplate(), 
    Nan::New<String>("playing").ToLocalChecked(), IsPlaying);
  Nan::SetAccessor(tmpl->InstanceTemplate(),
    Nan::New<String>("offset").ToLocalChecked(), Offset);
  Nan::SetAccessor(tmpl->InstanceTemplate(),
    Nan::New<String>("duration").ToLocalChecked(), Duration);

  Local<Function> func = Nan::GetFunction(tmpl).ToLocalChecked();
  Nan::Set(target, Nan::New("PlayWrap").ToLocalChecked(), func);
}

NAN_METHOD(PlayWrap::New) {
  PlayWrap* handle;
  if (info.Length() >= 2) {
    v8::String::Utf8Value type(info[0]);
    handle = new PlayWrap((const char*)*type);
    handle->mPlayer->setListener((MediaPlayerListener*)handle);
    handle->callback = new Nan::Callback(info[1].As<Function>());
  } else {
    handle = new PlayWrap();
    handle->mPlayer->setListener((MediaPlayerListener*)handle);
    handle->callback = new Nan::Callback(info[0].As<Function>());
  }
  handle->Wrap(info.This());
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(PlayWrap::SetDataSource) {
  PlayWrap* player = Nan::ObjectWrap::Unwrap<PlayWrap>(info.This());
  v8::String::Utf8Value url(info[0]);
  player->mPlayer->setDataSource(*url, nullptr);
  player->mPlayer->prepareAsync();
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(PlayWrap::Start) {
  PlayWrap* player = Nan::ObjectWrap::Unwrap<PlayWrap>(info.This());
  if (player->destroyed) {
    fprintf(stderr, "this player has been destroyed, just return\n");
    return;
  }
  player->mPlayer->start();
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(PlayWrap::Stop) {
  PlayWrap* player = Nan::ObjectWrap::Unwrap<PlayWrap>(info.This());
  if (player->destroyed) {
    fprintf(stderr, "this player has been destroyed, just return\n");
  } else {
    player->mPlayer->stop();
    player->destroy();
  }
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(PlayWrap::Pause) {
  PlayWrap* player = Nan::ObjectWrap::Unwrap<PlayWrap>(info.This());
  if (player->destroyed) {
    fprintf(stderr, "this player has been destroyed, just return\n");
  } else {
    player->mPlayer->pause();
  }
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(PlayWrap::Resume) {
  PlayWrap* player = Nan::ObjectWrap::Unwrap<PlayWrap>(info.This());
  if (player->destroyed) {
    fprintf(stderr, "this player has been destroyed, just return\n");
  } else {
    player->mPlayer->start();
  }
  info.GetReturnValue().Set(info.This());
}

NAN_METHOD(PlayWrap::Seek) {
  if (info.Length() == 0) {
    return Nan::ThrowTypeError("the pos is required in ms.");
  }
  PlayWrap* player = Nan::ObjectWrap::Unwrap<PlayWrap>(info.This());
  if (player->mPlayer->isPlaying()) {
    int pos = info[0]->NumberValue();
    player->mPlayer->seekTo(pos);
  }
  info.GetReturnValue().Set(info.This());
}

NAN_PROPERTY_GETTER(PlayWrap::IsPlaying) {
  PlayWrap* player = Nan::ObjectWrap::Unwrap<PlayWrap>(info.This());
  if (player->destroyed) {
    info.GetReturnValue().Set(Nan::False());
  } else {
    info.GetReturnValue().Set(Nan::New(player->mPlayer->isPlaying()));
  }
}

NAN_PROPERTY_GETTER(PlayWrap::Offset) {
  PlayWrap* player = Nan::ObjectWrap::Unwrap<PlayWrap>(info.This());
  int offset;
  if (player->destroyed) {
    offset = 0;
  } else {
    player->mPlayer->getCurrentPosition(&offset);
  }
  info.GetReturnValue().Set(Nan::New(offset));
}

NAN_PROPERTY_GETTER(PlayWrap::Duration) {
  PlayWrap* player = Nan::ObjectWrap::Unwrap<PlayWrap>(info.This());
  int duration;
  if (player->destroyed) {
    duration = 0;
  } else {
    player->mPlayer->getDuration(&duration);
  }
  info.GetReturnValue().Set(Nan::New(duration));
}

void InitModule(Handle<Object> target) {
  PlayWrap::Init(target);
}

NODE_MODULE(rplay, InitModule);
