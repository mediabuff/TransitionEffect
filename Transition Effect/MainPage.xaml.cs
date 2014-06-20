using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices.WindowsRuntime;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.Media.Core;
using Windows.Media.MediaProperties;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=391641

namespace Transition_Effect
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            LoadVideos();
            LoadEffects();

            this.InitializeComponent();

            this.DataContext = this;
            this.NavigationCacheMode = NavigationCacheMode.Required;
        }

        TestProj.Class1 c1;
        private void Apply_Effect(object sender, RoutedEventArgs e)
        {
            if (CurrentEffect == null || CurrentVideo == null) return;

            string SecondVideo = VideoList.IndexOf(CurrentVideo) == VideoList.Count - 1 ? VideoList[0] : VideoList[VideoList.IndexOf(CurrentVideo) + 1 ];

            /*PropertySet ps = new PropertySet();
			ps.Add("FadeDuration", 20000);
			ps.Add("FadeOutStart", 34000 - 5000);*/

            Video.RemoveAllEffects();
            //Video.AddVideoEffect("GrayscaleTransform.GrayscaleEffect", true, null);
            //Video.AddVideoEffect("FadeTransform.FadeEffect", true, ps);

            c1 = new TestProj.Class1();

            if (!c1.IsInitialized())
            {
                int x = 0; //break on error
            }

            if (!c1.SetVideo(0, CurrentVideo))
            {
                int x = 0; //break on error
            }

            if (!c1.SetVideo(1, CurrentVideo))
            {
                int x = 0; //break on error
            }

            int width = c1.GetWidth();
            int height = c1.GetHeight();
            int numerator = c1.GetFrameRate();

            VideoEncodingProperties videoProperties = VideoEncodingProperties.CreateUncompressed(MediaEncodingSubtypes.Nv12, (uint)width, (uint)height);
            var _videoDesc = new VideoStreamDescriptor(videoProperties);
            _videoDesc.EncodingProperties.FrameRate.Numerator = (uint)numerator;
            _videoDesc.EncodingProperties.FrameRate.Denominator = 1;
            _videoDesc.EncodingProperties.Bitrate = (uint)(numerator * width * (height + height / 2)); // NV12

           var _mss = new MediaStreamSource(_videoDesc);
            TimeSpan spanBuffer = new TimeSpan(0, 0, 0, 0, 250);
            _mss.BufferTime = spanBuffer;
            //_mss.Starting += _mss_Starting;
            _mss.SampleRequested += _mss_SampleRequested;

            //Video.CurrentStateChanged += mediaPlayer_CurrentStateChanged;
            Video.SetMediaStreamSource(_mss);
    
        }

        void _mss_SampleRequested(Windows.Media.Core.MediaStreamSource sender, MediaStreamSourceSampleRequestedEventArgs args)
        {
            c1.GenerateSample(args.Request);
        }

        private void LoadEffects()
        {
            EffectList = new ObservableCollection<Effect>();
            EffectList.Add(new Effect() { Name = "Black" });
            EffectList.Add(new Effect() { Name = "White" });
            EffectList.Add(new Effect() { Name = "Black Fading" });
            EffectList.Add(new Effect() { Name = "White Fading" });
            EffectList.Add(new Effect() { Name = "Right Left" });
            EffectList.Add(new Effect() { Name = "Top Bottom" });
            EffectList.Add(new Effect() { Name = "Bottom Top" });
            EffectList.Add(new Effect() { Name = "Quick" });
            EffectList.Add(new Effect() { Name = "ZoomIn" });
            EffectList.Add(new Effect() { Name = "ZoomOut" });
       }

        private void LoadVideos()
        {
            VideoList = new ObservableCollection<string>();

            VideoList.Add(@"ms-appx:///Assets/Videos/big_buck_bunny_trailer_480p_high.mp4");
            VideoList.Add(@"ms-appx:///Assets/Videos/1.wmv");
            VideoList.Add(@"ms-appx:///Assets/Videos/Mortal Kombat Legacy.mp4");

        }

        public string CurrentVideo { get; set; }
        public Effect CurrentEffect { get; set; }

        public ObservableCollection<string>  VideoList { get; set; }
        public ObservableCollection<Effect> EffectList { get; set; }
  
    }
}
