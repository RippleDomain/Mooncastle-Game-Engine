using MooncastleEditor.Content;
using MooncastleEditor.DllWrappers;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;

namespace MooncastleEditor.Editors
{
    class TextureEditor : ViewModelBase, IAssetEditor
    {
        private readonly List<List<List<BitmapSource>>> _sliceBitmaps = new();
        private List<List<List<Slice>>> _slices;

        public ICommand SetAllChannelsCommand { get; init; }
        public ICommand SetChannelCommand { get; init; }
        public ICommand RegenerateBitmapsCommand { get; init; }

        private AssetEditorState _state;
        public AssetEditorState State
        {
            get => _state;
            private set
            {
                if (_state != value)
                {
                    _state = value;
                    OnPropertyChanged(nameof(State));
                }
            }
        }

        public Guid AssetGuid { get; private set; }

        private bool _isRedChannelSelected = true;
        public bool IsRedChannelSelected
        {
            get => _isRedChannelSelected;
            set
            {
                if (_isRedChannelSelected != value)
                {
                    _isRedChannelSelected = value;
                    OnPropertyChanged(nameof(IsRedChannelSelected));
                    SetImageChannel();
                }
            }
        }

        private bool _isGreenChannelSelected = true;
        public bool IsGreenChannelSelected
        {
            get => _isGreenChannelSelected;
            set
            {
                if (_isGreenChannelSelected != value)
                {
                    _isGreenChannelSelected = value;
                    OnPropertyChanged(nameof(IsGreenChannelSelected));
                    SetImageChannel();
                }
            }
        }

        private bool _isBlueChannelSelected = true;
        public bool IsBlueChannelSelected
        {
            get => _isBlueChannelSelected;
            set
            {
                if (_isBlueChannelSelected != value)
                {
                    _isBlueChannelSelected = value;
                    OnPropertyChanged(nameof(IsBlueChannelSelected));
                    SetImageChannel();
                }
            }
        }

        private bool _isAlphaChannelSelected = true;
        public bool IsAlphaChannelSelected
        {
            get => _isAlphaChannelSelected;
            set
            {
                if (_isAlphaChannelSelected != value)
                {
                    _isAlphaChannelSelected = value;
                    OnPropertyChanged(nameof(IsAlphaChannelSelected));
                    SetImageChannel();
                }
            }
        }

        public Color Channels => new()
        {
            ScR = IsRedChannelSelected ? 1.0f : 0.0f,
            ScG = IsGreenChannelSelected ? 1.0f : 0.0f,
            ScB = IsBlueChannelSelected ? 1.0f : 0.0f,
            ScA = IsAlphaChannelSelected ? 1.0f : 0.0f
        };

        public float Stride => (float?)(SelectedSliceBitmap?.Format.BitsPerPixel / 8) ?? 1.0f;

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
                    SetSelectedBitmap();
                    SetImageChannel();
                }
            }
        }

        public int MaxMipIndex => _sliceBitmaps.Any() && _sliceBitmaps.First().Any() ? _sliceBitmaps.First().Count - 1 : 0;
        public int MaxArrayIndex => _sliceBitmaps.Any() ? _sliceBitmaps.Count - 1 : 0;
        public int MaxDepthIndex => _sliceBitmaps.Any() && _sliceBitmaps.First().Any() && _sliceBitmaps.First().First().Any() ?
            _sliceBitmaps.ElementAtOrDefault(ArrayIndex).ElementAtOrDefault(MipIndex).Count - 1 : 0;

        private int _arrayIndex;
        public int ArrayIndex
        {
            get => Math.Min(MaxArrayIndex, _arrayIndex);
            set
            {
                value = Math.Min(value, MaxArrayIndex);
                if (_arrayIndex != value)
                {
                    _arrayIndex = value;
                    OnPropertyChanged(nameof(ArrayIndex));
                    SetSelectedBitmap();
                    SetImageChannel();
                }
            }
        }

        private int _mipIndex;
        public int MipIndex
        {
            get => Math.Min(MaxMipIndex, _mipIndex);
            set
            {
                value = Math.Min(value, MaxMipIndex);
                if (_mipIndex != value)
                {
                    _mipIndex = value;
                    DepthIndex = _depthIndex;
                    OnPropertyChanged(nameof(MipIndex));
                    OnPropertyChanged(nameof(MaxDepthIndex));
                    SetSelectedBitmap();
                    SetImageChannel();
                }
            }
        }

        private int _depthIndex;
        public int DepthIndex
        {
            get => Math.Min(MaxDepthIndex, _depthIndex);
            set
            {
                value = Math.Min(value, MaxDepthIndex);
                if (_depthIndex != value)
                {
                    _depthIndex = value;
                    OnPropertyChanged(nameof(DepthIndex));
                    SetSelectedBitmap();
                    SetImageChannel();
                }
            }
        }

        public BitmapSource SelectedSliceBitmap => _sliceBitmaps.ElementAtOrDefault(ArrayIndex)?.ElementAtOrDefault(MipIndex)?.ElementAtOrDefault(DepthIndex);
        public Slice SelectedSlice => Texture?.Slices?.ElementAtOrDefault(ArrayIndex)?.ElementAtOrDefault(MipIndex)?.ElementAtOrDefault(DepthIndex);
        public long DataSize => Texture?.Slices?.Sum(x => x.Sum(y => y.Sum(z => z.RawContent.LongLength))) ?? 0;

        private void SetSelectedBitmap()
        {
            OnPropertyChanged(nameof(SelectedSliceBitmap));
            OnPropertyChanged(nameof(SelectedSlice));
            OnPropertyChanged(nameof(DataSize));
        }

        private void SetImageChannel()
        {
            OnPropertyChanged(nameof(Channels));
            OnPropertyChanged(nameof(Stride));
        }

        private void OnSetAllChannelsCommand(object commandParam)
        {
            _isRedChannelSelected = true;
            _isGreenChannelSelected = true;
            _isBlueChannelSelected = true;
            _isAlphaChannelSelected = true;

            OnPropertyChanged(nameof(IsRedChannelSelected));
            OnPropertyChanged(nameof(IsGreenChannelSelected));
            OnPropertyChanged(nameof(IsBlueChannelSelected));
            OnPropertyChanged(nameof(IsAlphaChannelSelected));

            SetImageChannel();
        }

        private void OnSetChannelCommand(string commandParam)
        {
            if (!Keyboard.Modifiers.HasFlag(ModifierKeys.Shift))
            {
                _isRedChannelSelected = false;
                _isGreenChannelSelected = false;
                _isBlueChannelSelected = false;
                _isAlphaChannelSelected = false;

                OnPropertyChanged(nameof(IsRedChannelSelected));
                OnPropertyChanged(nameof(IsGreenChannelSelected));
                OnPropertyChanged(nameof(IsBlueChannelSelected));
                OnPropertyChanged(nameof(IsAlphaChannelSelected));
            }

            switch (commandParam)
            {
                case "0": IsRedChannelSelected = !IsRedChannelSelected; break;
                case "1": IsGreenChannelSelected = !IsGreenChannelSelected; break;
                case "2": IsBlueChannelSelected = !IsBlueChannelSelected; break;
                case "3": IsAlphaChannelSelected = !IsAlphaChannelSelected; break;
            }
        }

        private void OnRegenerateBitmapsCommand(bool isNormalMap)
        {
            GenerateSliceBitmaps(isNormalMap);
            OnPropertyChanged(nameof(SelectedSliceBitmap));
            SetImageChannel();
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

                await SetMIPMaps(texture);

                Texture = texture;
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Debug.WriteLine($"Failed to set texture for use in texture editor. File: {info.FullPath}");

                Texture = new();
            }
            finally { State = AssetEditorState.Done; }
        }

        private async Task SetMIPMaps(Texture texture)
        {
            try
            {
                await Task.Run(() => _slices = texture.TextureImportSettings.Compress ? ContentToolsAPI.Decompress(texture) : texture.Slices);

                Debug.Assert(_slices?.Any() == true && _slices.First()?.Any() == true);
                GenerateSliceBitmaps(texture.IsNormalMap);
                OnPropertyChanged(nameof(Texture));
                OnPropertyChanged(nameof(DataSize));
            }
            catch (Exception ex)
            {
                Debug.WriteLine(ex.Message);
                Debug.WriteLine($"Failed to load mipmaps from {texture.FileName}");
            }
        }

        private void GenerateSliceBitmaps(bool isNormalMap)
        {
            _sliceBitmaps.Clear();
            foreach (var arraySlice in _slices)
            {
                List<List<BitmapSource>> mipmapsBitmaps = new();
                foreach (var mipLevel in arraySlice)
                {
                    List<BitmapSource> sliceBitmap = new();
                    foreach (var slice in mipLevel)
                    {
                        var image = BitmapHelper.ImageFromSlice(slice, isNormalMap);
                        Debug.Assert(image != null);
                        sliceBitmap.Add(image);
                    }
                    mipmapsBitmaps.Add(sliceBitmap);
                }

                _sliceBitmaps.Add(mipmapsBitmaps);
            }

            OnPropertyChanged(nameof(MaxMipIndex));
            OnPropertyChanged(nameof(MaxArrayIndex));
            OnPropertyChanged(nameof(MaxDepthIndex));
        }

        public TextureEditor()
        {
            SetAllChannelsCommand = new RelayCommand<string>(OnSetAllChannelsCommand);
            SetChannelCommand = new RelayCommand<string>(OnSetChannelCommand);
            RegenerateBitmapsCommand = new RelayCommand<bool>(OnRegenerateBitmapsCommand);
        }
    }
}
