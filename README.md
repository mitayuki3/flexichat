# FlexiChat

Qt 製のデスクトップアプリで、ローカル / クラウドの AI とチャットしながら、TTS（Text-to-Speech）で音声合成・再生できます。

- LM Studio の OpenAI 互換 API を経由した LLM チャット
- OpenAI 互換 TTS API（[Irodori TTS](https://github.com/mitayuki3/Irodori-TTS) など）を使った音声合成と再生
- システムプロンプト・サンプリングパラメータをまとめた「プロファイル」管理

## ビルド要件

- CMake 3.16 以上
- C++17 対応コンパイラ
- **Qt 6.7 以上**（必須）
  - `QMediaPlayer::playingChanged` は Qt 6.5+
  - `QCheckBox::checkStateChanged` は Qt 6.7+
  - Ubuntu 22.04 標準の Qt 6.2 ではビルドできません。Ubuntu 24.04 以降、もしくは [Qt 公式インストーラ](https://www.qt.io/download-qt-installer) や `aqtinstall` で新しめの Qt6 を導入してください。
- 必要な Qt6 コンポーネント: `Widgets` / `Gui` / `Network` / `Multimedia` / `MultimediaWidgets` / `LinguistTools`

## ビルド

```sh
cmake -S . -B build
cmake --build build -j
```

`Qt6_DIR` が見つからない場合は明示的に渡してください:

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.7.0/gcc_64
```

## 実行

```sh
./build/FlexiChat
```

## 設定

設定は QSettings に永続化されます（プラットフォームごとの標準位置: Linux なら `~/.config/FlexiChat/FlexiChat.conf` 等）。アプリ内の「設定」ダイアログから編集できます。

主な TTS 関連キー:

| キー | 説明 |
| --- | --- |
| `Tts/ApiKey` | TTS API のキー |
| `Tts/Model` | モデル名（例: `irodori-tts-500m-v2`） |
| `Tts/Voice` | ボイス名（例: `alloy`） |
| `Tts/Instructions` | スタイル指示 |
| `Tts/AutoPlay` | 応答を自動再生するか |
| `Tts/BaseUrl` | TTS API のベース URL |
| `Tts/OutputDir` | 生成された音声ファイルの保存先 |

## TTS タブ

- 「生成」ボタンで入力テキストを音声合成し、音声ファイルがリストに追加されます。
- 自動再生が有効な場合、AI 応答受信時に自動で合成・再生されます。
- voice コンボボックスでボイスを切り替えられます。変更は即座に反映され、`Tts/Voice` として設定に保存されます。

## ライセンス

[LICENSE](LICENSE) を参照してください。
