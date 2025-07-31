using MooncastleEditor.Content;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MooncastleEditor.Editors
{
    class TextureEditor : ViewModelBase, IAssetEditor
    {
        private AssetEditorState _state;
        public AssetEditorState State
        {
            get => _state;
            set
            {
                if (_state != value)
                {
                    _state = value;
                    OnPropertyChanged(nameof(State));
                }
            }
        }

        public Guid AssetGuid { get; private set; }

        Asset IAssetEditor.Asset => Texture;

        private Texture _texture;
        public Texture Texture
        {
            get => _texture;
            private set
            {
                if (_texture != value)
                {
                    _texture = value;
                    OnPropertyChanged(nameof(Texture));
                }
            }
        }

        public async void SetAsset(AssetInfo info)
        {
            try
            {
                AssetGuid = info.Guid;
                Texture = null;

                Debug.Assert(info != null && File.Exists(info.FullPath));

                var texture = new Texture();
                State = AssetEditorState.Loading;

                await Task.Run(() =>
                {
                    texture.Load(info.FullPath);
                });

                Texture = texture;
            }
            catch (Exception ex)
            {
                Debug.Write(ex.Message);
                Debug.WriteLine($"Failed to set texture for use in texture editor. File: {info.FullPath}");

                Texture = new();
            }
            finally { State = AssetEditorState.Done; }
        }
    }
}
