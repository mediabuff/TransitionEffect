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
using AdvancedMediaSource;
using Transition_Effect.Common;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=391641

namespace Transition_Effect
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public sealed partial class MainPage : Page
	{
		CAdvancedMediaSource ams;
		MediaStreamSource mss;
		VideoStreamDescriptor videoDesc;

        NavigationHelper navigationHelper;

		public MainPage()
		{
			LoadVideos();
			LoadEffects();

			this.InitializeComponent();

			this.DataContext = this;
			this.NavigationCacheMode = NavigationCacheMode.Required;
            navigationHelper = new NavigationHelper(this);
		}

		private void Apply_Effect(object sender, RoutedEventArgs e)
		{
			if (CurrentEffect == null || CurrentVideo == null) return;

			string SecondVideo = VideoList.IndexOf(CurrentVideo) == VideoList.Count - 1 ? VideoList[0] : VideoList[VideoList.IndexOf(CurrentVideo) + 1];

			ams = new CAdvancedMediaSource();

			if(!ams.IsInitialized())
			{
				int x = 0; //break on error
			}

			if(!ams.AddVideo(CurrentVideo, EIntroType.FadeIn, 5000, EOutroType.FadeOut, 5000, EVideoEffect.None))
			{
				int x = 0;
			}


			if (!ams.AddVideo("ms-appx:///Assets/Videos/Mortal Kombat Legacy.mp4", EIntroType.FadeIn, 5000, EOutroType.FadeOut, 5000, EVideoEffect.None))
			{
				int x = 0;
			}
			/*c1 = new TestProj.Class1();

			

			if (!c1.SetVideo(0, CurrentVideo))
			{
				int x = 0; //break on error
			}

			if (!c1.SetVideo(1, CurrentVideo))
			{
				int x = 0; //break on error
			}
			*/
			SVideoData vd;

			ams.GetVideoData(out vd);

			VideoEncodingProperties videoProperties = VideoEncodingProperties.CreateUncompressed(MediaEncodingSubtypes.Bgra8, vd.Width, vd.Height);
			videoDesc = new VideoStreamDescriptor(videoProperties);
			videoDesc.EncodingProperties.FrameRate.Numerator = vd.Numerator;
			videoDesc.EncodingProperties.FrameRate.Denominator = vd.Denominator;
			videoDesc.EncodingProperties.Bitrate = (uint)(((UInt64) vd.Numerator * vd.Width * vd.Height * 4) / vd.Denominator);

			if(vd.HasAudio)
			{
				AudioEncodingProperties audioProperties = AudioEncodingProperties.CreatePcm(vd.ASampleRate, vd.AChannelCount, vd.ABitsPerSample);
				AudioStreamDescriptor audioDesc = new AudioStreamDescriptor(audioProperties);

				mss = new MediaStreamSource(videoDesc, audioDesc);
			}
			else
			{
				mss = new MediaStreamSource(videoDesc);
			}
			
			TimeSpan spanBuffer = new TimeSpan(0, 0, 0, 0, 250);
			mss.BufferTime = spanBuffer;
			mss.Starting += MSS_Starting;
			mss.SampleRequested += MSS_SampleRequested;

			//Video.CurrentStateChanged += mediaPlayer_CurrentStateChanged;
			Video.SetMediaStreamSource(mss);
			
		}

		void MSS_Starting(Windows.Media.Core.MediaStreamSource sender, MediaStreamSourceStartingEventArgs args)
		{
			if(!ams.OnStart(videoDesc))
			{
				int x = 0;
			}

			args.Request.SetActualStartPosition(new TimeSpan(0));
		}

		void MSS_SampleRequested(Windows.Media.Core.MediaStreamSource sender, MediaStreamSourceSampleRequestedEventArgs args)
		{
			if (args.Request.StreamDescriptor is VideoStreamDescriptor)
			{
				ams.GenerateVideoSample(args.Request);
			}
			else if (args.Request.StreamDescriptor is AudioStreamDescriptor)
			{
				ams.GenerateAudioSample(args.Request);
			}
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

		public ObservableCollection<string> VideoList { get; set; }
		public ObservableCollection<Effect> EffectList { get; set; }

        private void Playback_click(object sender, RoutedEventArgs e)
        {
           
            Frame.Navigate(typeof(Playback));
        }


        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
            this.navigationHelper.OnNavigatedTo(e);
        }

        protected override void OnNavigatedFrom(NavigationEventArgs e)
        {
            this.navigationHelper.OnNavigatedFrom(e);
        }
	}
}

