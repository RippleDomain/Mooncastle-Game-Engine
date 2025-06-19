using MooncastleEditor.DllWrappers;
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Interop;

namespace MooncastleEditor.Utilities
{
    class RenderSurfaceHost : HwndHost
    {
        private readonly int _width = 800;
        private readonly int _height = 600;
        private IntPtr renderWindowHandle = IntPtr.Zero;

        public int SurfaceId { get; private set; } = ID.InvalidID;

        public RenderSurfaceHost(double width, double height)
        {
            _width = (int)width;
            _height = (int)height;
        }

        protected override HandleRef BuildWindowCore(HandleRef hwndParent)
        {
            SurfaceId = EngineAPI.CreateRenderSurface(hwndParent.Handle, _width, _height);
            Debug.Assert(ID.isValid(SurfaceId));
            renderWindowHandle = EngineAPI.GetWindowHandle(SurfaceId);
            Debug.Assert(renderWindowHandle != IntPtr.Zero);

            return new HandleRef(this, renderWindowHandle);
        }

        protected override void DestroyWindowCore(HandleRef hwnd)
        {
            EngineAPI.RemoveRenderSurface(SurfaceId);
            SurfaceId = ID.InvalidID;
            renderWindowHandle = IntPtr.Zero;
        }
    }
}
